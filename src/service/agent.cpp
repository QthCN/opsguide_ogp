//
// Created by thuanqin on 16/5/24.
//
#include "service/agent.h"

#include <cstdint>
#include <thread>
#include <vector>

#include "boost/asio.hpp"
#include "boost/bind.hpp"

#include "common/config.h"
#include "common/log.h"
#include "common/utils.h"

void ControllerSession::send_msg(msg_ptr msg) {
    /*
     * 发送消息。该方法会将消息放到发送队列中,然后判断目前是否正在发送消息,如果是则直接返回,否则开始发送。
     * 发送开始后,每发送完一个消息都会去检查发送队列是否是空的,如果是则结束发送,否则继续发送并重复此过程。
     * */
    if (!valid()) return;
    auto take_care_of_send = false;
    msg_lock.lock();
    messages.push(msg);
    if (!sending) {
        take_care_of_send = true;
        sending = true;
    } else {
        take_care_of_send = false;
    }
    msg_lock.unlock();
    if (!take_care_of_send) {
        // 已经有worker在负责发送消息了
        return;
    }
    // 需要负责发送消息
    agent_service->begin_write(shared_from_this());
}

void ControllerSession::invalid_sess() {
    if (!valid()) return;
    stop();
}

AgentService::AgentService(unsigned int thread_num, const std::string &controller_address,
                                     unsigned int controller_port,
                                     BaseController *controller):thread_num(thread_num),
                                                                 controller_address(controller_address),
                                                                 controller_port(controller_port),
                                                                 controller(controller) {
    controller->init();

    start_connect();
}

void AgentService::invalid_and_remove_sess(controller_sess_ptr controller_sess) {
    sess_lock.lock();
    if (controller_sess->valid()) {
        // 通知controller移除此session相关的信息
        controller->invalid_sess(controller_sess);
        // 通知session清理相关资源
        controller_sess->invalid_sess();
    }
    // 删除对session的引用
    for (auto k=controller_sessions.begin(); k!=controller_sessions.end(); k++) {
        if ((*k)->get_address() == controller_sess->get_address()
            && (*k)->get_port() == controller_sess->get_port()) {
            controller_sessions.erase(k);
            break;
        }
    }
    sess_lock.unlock();
}

void AgentService::handle_connect(controller_sess_ptr controller_sess, boost::system::error_code error) {
    if (!error) {
        auto controller_remote_address = controller_sess->get_socket().remote_endpoint().address().to_string();
        auto controller_remote_port = controller_sess->get_socket().remote_endpoint().port();
        controller_sess->set_address(controller_remote_address);
        controller_sess->set_port(controller_remote_port);
        LOG_INFO("Connected to controller: " << controller_remote_address << ":" << controller_remote_port);

        try {
            controller->associate_sess(controller_sess);
            // 通知Controller我是一个docker agent
            controller_sess->send_msg(std::make_shared<Message>(
                                        MsgType::DA_DOCKER_SAY_HI,
                                        new char[0],
                                        0
                                ));
        } catch (...) {
            LOG_ERROR("Session association Exception!!!")
            invalid_and_remove_sess(controller_sess);
            reconnect();
        }
        start_read(controller_sess);
    } else {
        LOG_ERROR("Connect to controller error: " << error)
        invalid_and_remove_sess(controller_sess);
        reconnect();
    }
}

void AgentService::reconnect() {
    sess_lock.lock();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    if (controller_sessions.size() == 0) {
        LOG_INFO("Reconnect to controller now.")
        start_connect();
    }
    sess_lock.unlock();
}

void AgentService::start_connect() {
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(controller_address), controller_port);
    controller_sess_ptr controller_sess = std::make_shared<ControllerSession>(io_service, this, controller);
    controller_sess->set_read_timeout(static_cast<unsigned int>(config_mgr.get_item("agent_read_timeout")->get_int()));
    controller_sess->set_write_timeout(static_cast<unsigned int>(config_mgr.get_item("agent_write_timeout")->get_int()));
    controller_sessions.push_back(controller_sess);
    controller_sess->get_socket().async_connect(ep,
                                                boost::bind(&AgentService::handle_connect, this,
                                                            controller_sess, boost::asio::placeholders::error));
}

void AgentService::run() {
    LOG_INFO("AgentService run. Thread num: " << thread_num
             << " Controller Address: " << controller_address << " Port: " << controller_port)
    std::vector<std::thread> cs_threads;
    // 监听线程
    for (unsigned int i=0; i<thread_num; i++) {
        std::thread t([this](){this->io_service.run();});
        cs_threads.push_back(std::move(t));
    }
    // 心跳线程
    std::thread t([this](){this->controller->send_heartbeat();});
    cs_threads.push_back(std::move(t));

    // 状态同步线程
    std::thread t2([this](){this->controller->sync();});
    cs_threads.push_back(std::move(t2));

    for (auto t = cs_threads.begin(); t!=cs_threads.end();t++) {
        t->join();
    }
}


size_t AgentService::read_completion_handler(controller_sess_ptr controller_sess, boost::system::error_code const &error,
                                                  size_t bytes_transferred) {
    if (!error) {
        if (controller_sess->get_cmsg_length() == 0) {
            if (controller_sess->get_read_buf().size() >= msg_length_hdr_size) {
                return 0;
            }
        } else {
            if (controller_sess->get_read_buf().size() >= controller_sess->get_cmsg_length()) {
                return 0;
            }
        }
        return 1024;
    } else if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
        // 对端中断了连接
        LOG_ERROR("read_completion_handler error: " << error)
        invalid_and_remove_sess(controller_sess);
        reconnect();
        return 0;
    } else {
        LOG_ERROR("read_completion_handler error: " << error)
        invalid_and_remove_sess(controller_sess);
        reconnect();
        return 0;
    }
}

void AgentService::start_read(controller_sess_ptr controller_sess) {
    if (!controller_sess->valid()) return;
    // 设置新的超时器
    controller_sess->get_read_deadline_timer().expires_from_now(
            boost::posix_time::seconds(controller_sess->get_read_timeout()));
    controller_sess->get_read_deadline_timer().async_wait(
            controller_sess->get_strand().wrap(
                    boost::bind(&AgentService::handle_read_timeout,
                                this, controller_sess,
                                boost::asio::placeholders::error)));
    // 将读取的socket放入poll中
    boost::asio::async_read(
            controller_sess->get_socket(),
            controller_sess->get_read_buf(),
            boost::bind(&AgentService::read_completion_handler,
                        this,
                        controller_sess,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred),
            controller_sess->get_strand().wrap(
                    boost::bind(&AgentService::handle_read,
                                this,
                                controller_sess,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred))
    );
}

void AgentService::handle_read_timeout(controller_sess_ptr controller_sess, boost::system::error_code const &error) {
    if (!error) {
        if (controller_sess->get_read_deadline_timer().expires_at() >= boost::asio::deadline_timer::traits_type::now()) {
            // 计时器时间被重置过,因此没有超时.此超时器被调用是由于其在read函数处理期间排队在队列中

        } else {
            LOG_WARN("Agent read time out")
            // 针对读取超时进行相关操作
            invalid_and_remove_sess(controller_sess);
            reconnect();
        }
    } else if (error == boost::asio::error::operation_aborted) {
        // 定时器被重置超时时间
        ;
    } else {
        LOG_ERROR("Read time out error: " << error)
        invalid_and_remove_sess(controller_sess);
        reconnect();
    }

}

void AgentService::handle_write_timeout(controller_sess_ptr controller_sess, boost::system::error_code const &error) {
    if (!error) {
        if (controller_sess->get_write_deadline_timer().expires_at() >= boost::asio::deadline_timer::traits_type::now()) {
            // 计时器时间被重置过,因此没有超时.此超时器被调用是由于其在read函数处理期间排队在队列中

        } else {
            LOG_WARN("Agent write time out")
            // 针对写入超时进行相关操作
            invalid_and_remove_sess(controller_sess);
            reconnect();
        }
    } else if (error == boost::asio::error::operation_aborted) {
        // 定时器被重置超时时间
        ;
    } else {
        LOG_ERROR("Write time out error: " << error)
        // 如果必要要对socket进行关闭操作,否则socket会一直在poll中write
        invalid_and_remove_sess(controller_sess);
        reconnect();
    }

}

void AgentService::handle_write(controller_sess_ptr controller_sess, boost::system::error_code const &error,
                                     size_t bytes_transferred) {
    if (!error) {
        if (!controller_sess->valid()) {
            LOG_WARN("Agent not valid")
            return;
        }

        // 继续发送队列中的消息
        start_write(controller_sess);
    } else if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
        // 对端中断了连接
        invalid_and_remove_sess(controller_sess);
        reconnect();
    } else {
        LOG_ERROR("Write data error: " << error)
        invalid_and_remove_sess(controller_sess);
        reconnect();
    }
}

void AgentService::handle_read(controller_sess_ptr controller_sess, boost::system::error_code const &error,
                                    size_t bytes_transferred) {
    if (!error) {
        if (!controller_sess->valid()) {
            LOG_WARN("Agent not valid")
            return;
        }

        bool enough_data = true;
        while (enough_data) {
            /*
             * 报文格式为: 4字节余下部分总长度 + 2字节msg类型 + protobuf报文
             * 例如收到一个10字节的报文,则开始的4个字节为0x00000006,接着是2字节的类型,如0x0001,在接着是4字节的profobuf内容
             * */

            // 获取长度信息
            if (controller_sess->get_cmsg_length() == 0) {
                char ld[msg_length_hdr_size];
                // 该操作会consume read_buf的字节
                controller_sess->get_read_buf().sgetn(ld, msg_length_hdr_size);
                auto dl = get_u32_from_4_u8(uint8_t(ld[0]), uint8_t(ld[1]), uint8_t(ld[2]), uint8_t(ld[3]));
                controller_sess->set_cmsg_length(dl);
            }
            // 根据长度信息判断该报文是否包含了完整的数据部分,如果不是则不处理,否则处理这部分数据
            if (controller_sess->get_read_buf().size() >= controller_sess->get_cmsg_length()) {
                char msg_type_[msg_type_hdr_size];
                controller_sess->get_read_buf().sgetn(msg_type_, msg_type_hdr_size);
                char *msg_body = new char[controller_sess->get_cmsg_length()-msg_type_hdr_size];
                controller_sess->get_read_buf().sgetn(msg_body, controller_sess->get_cmsg_length()-msg_type_hdr_size);
                // 处理消息
                auto msg_type = static_cast<unsigned int>(get_u32_from_2_u8(uint8_t(
                        msg_type_[0]), uint8_t(msg_type_[1])));

                // 交由controller处理消息
                auto msg = std::make_shared<Message>(static_cast<MsgType>(msg_type), msg_body,
                                                     controller_sess->get_cmsg_length()-msg_type_hdr_size);
                try {
                    controller->handle_msg(controller_sess, msg);
                } catch (...) {
                    // controller处理消息异常
                }
                controller_sess->set_cmsg_length(0);

                if (!controller_sess->valid()) {
                    enough_data = false;
                }

                if (controller_sess->get_read_buf().size() < msg_length_hdr_size) {
                    enough_data = false;
                }
            } else {
                enough_data = false;
            }
        }

        // 继续监听
        start_read(controller_sess);
    } else if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
        // 对端中断了连接
        invalid_and_remove_sess(controller_sess);
        reconnect();
    } else {
        LOG_ERROR("Read data error: " << error)
        invalid_and_remove_sess(controller_sess);
        reconnect();
    }
}

void AgentService::begin_write(controller_sess_ptr controller_sess) {
    start_write(controller_sess);
}

void AgentService::start_write(controller_sess_ptr controller_sess) {
    if (!controller_sess->valid()) return;
    msg_ptr msg = nullptr;
    controller_sess->get_msg_lock().lock();
    if (controller_sess->get_messages().empty()) {
        // 没有要发的消息了
        controller_sess->set_sending(false);
        controller_sess->get_msg_lock().unlock();
        controller_sess->get_write_deadline_timer().expires_at(boost::posix_time::pos_infin);
        return;
    }
    msg = controller_sess->get_messages().front();
    controller_sess->get_messages().pop();
    controller_sess->get_msg_lock().unlock();

    // 构建发送的write_buf
    if (controller_sess->get_write_buf().size() != 0) {
        LOG_WARN("Controller write buf is not empty!")
        controller_sess->get_write_buf().consume(controller_sess->get_write_buf().size());
    }

    auto msg_size = get_4_u8_from_u32(static_cast<uint32_t>(msg_type_hdr_size + msg->get_msg_body_size()));
    auto msg_type = get_2_u8_from_u32(static_cast<uint32_t>(msg->get_msg_type()));
    std::ostream os(&(controller_sess->get_write_buf()));
    os.write(msg_size, msg_length_hdr_size);
    os.write(msg_type, msg_type_hdr_size);
    os.write(msg->get_msg_body(), msg->get_msg_body_size());
    delete[] msg_size;
    delete[] msg_type;

    // 设置新的超时器
    controller_sess->get_write_deadline_timer().expires_from_now(
            boost::posix_time::seconds(controller_sess->get_write_timeout()));
    controller_sess->get_write_deadline_timer().async_wait(
            controller_sess->get_strand().wrap(
                    boost::bind(&AgentService::handle_write_timeout,
                                this, controller_sess,
                                boost::asio::placeholders::error)));
    // 将写入的socket放入poll中
    boost::asio::async_write(
            controller_sess->get_socket(),
            controller_sess->get_write_buf(),
            controller_sess->get_strand().wrap(
                    boost::bind(&AgentService::handle_write,
                                this,
                                controller_sess,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred))
    );
}

