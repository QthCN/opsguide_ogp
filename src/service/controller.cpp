//
// Created by thuanqin on 16/5/19.
//
#include "service/controller.h"

#include <cstdint>
#include <thread>
#include <vector>

#include "boost/asio.hpp"
#include "boost/bind.hpp"

#include "common/config.h"
#include "common/log.h"
#include "common/utils.h"

void AgentSession::send_msg(msg_ptr msg) {
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
    controller_service->begin_write(shared_from_this());
}

void AgentSession::invalid_sess() {
    if (!valid()) return;
    stop();
}

void ControllerService::invalid_and_remove_sess(agent_sess_ptr agent_sess) {
    sess_lock.lock();
    if (agent_sess->valid()) {
        // 通知controller移除此session相关的信息
        controller->invalid_sess(agent_sess);
        // 通知session清理相关资源
        agent_sess->invalid_sess();
    }
    // 删除对session的引用
    for (auto k=agent_sessions.begin(); k!=agent_sessions.end(); k++) {
        if ((*k)->get_address() == agent_sess->get_address()
            && (*k)->get_port() == agent_sess->get_port()) {
            agent_sessions.erase(k);
            break;
        }
    }
    sess_lock.unlock();
}

bool ControllerService::sess_exist(agent_sess_ptr agent_sess) {
    sess_lock.lock();
    auto ret = false;
    auto ip_address = agent_sess->get_address();
    auto port = agent_sess->get_port();
    for (auto s: agent_sessions) {
        if (s->get_address()==ip_address && s->get_port()==port) {
            ret = true;
            break;
        }
    }
    sess_lock.unlock();
    return ret;
}

bool ControllerService::sess_exist(const std::string &address, unsigned short port) {
    sess_lock.lock();
    auto ret = false;
    for (auto s: agent_sessions) {
        if (s->get_address()==address && s->get_port()==port) {
            ret = true;
            break;
        }
    }
    sess_lock.unlock();
    return ret;
}

ControllerService::ControllerService(unsigned int thread_num, const std::string &listen_address,
                                     unsigned int listen_port,
                                     BaseController *controller):thread_num(thread_num),
                                                                 listen_address(listen_address),
                                                                 listen_port(listen_port),
                                                                 controller(controller) {
    controller->init();

    if (listen_address == "0.0.0.0") {
        auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), listen_port);
        acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(io_service, endpoint);
    } else {
        auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(listen_address), listen_port);
        acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(io_service, endpoint);
    }

    start_accept();
}

void ControllerService::run() {
    LOG_INFO("ControllerService run. Thread num: " << thread_num
             << " Address: " << listen_address << " Port: " << listen_port)
    std::vector<std::thread> cs_threads;
    for (unsigned int i=0; i<thread_num; i++) {
        std::thread t([this](){this->io_service.run();});
        cs_threads.push_back(std::move(t));
    }
    for (auto t = cs_threads.begin(); t!=cs_threads.end();t++) {
        t->join();
    }
}

void ControllerService::start_accept() {
    agent_sess_ptr agent_sess = std::make_shared<AgentSession>(io_service, this, controller);
    agent_sess->set_read_timeout(static_cast<unsigned int>(config_mgr.get_item("controller_read_timeout")->get_int()));
    agent_sess->set_write_timeout(static_cast<unsigned int>(config_mgr.get_item("controller_write_timeout")->get_int()));
    acceptor->async_accept(agent_sess->get_socket(),
                           boost::bind(&ControllerService::handle_accept, this,
                                       agent_sess, boost::asio::placeholders::error));
}

size_t ControllerService::read_completion_handler(agent_sess_ptr agent_sess, boost::system::error_code const &error,
                                                  size_t bytes_transferred) {
    if (!error) {
        if (agent_sess->get_cmsg_length() == 0) {
            if (agent_sess->get_read_buf().size() >= msg_length_hdr_size) {
                return 0;
            }
        } else {
            if (agent_sess->get_read_buf().size() >= agent_sess->get_cmsg_length()) {
                return 0;
            }
        }
        return 1024;
    } else if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
        // 对端中断了连接
        invalid_and_remove_sess(agent_sess);
        return 0;
    } else {
        LOG_ERROR("read_completion_handler error: " << error)
        invalid_and_remove_sess(agent_sess);
        return 0;
    }
}

void ControllerService::start_read(agent_sess_ptr agent_sess) {
    if (!agent_sess->valid()) return;
    // 设置新的超时器
    agent_sess->get_read_deadline_timer().expires_from_now(
            boost::posix_time::seconds(agent_sess->get_read_timeout()));
    agent_sess->get_read_deadline_timer().async_wait(
            agent_sess->get_strand().wrap(
                    boost::bind(&ControllerService::handle_read_timeout,
                                this, agent_sess,
                                boost::asio::placeholders::error)));
    // 将读取的socket放入poll中
    boost::asio::async_read(
            agent_sess->get_socket(),
            agent_sess->get_read_buf(),
            boost::bind(&ControllerService::read_completion_handler,
                        this,
                        agent_sess,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred),
            agent_sess->get_strand().wrap(
                    boost::bind(&ControllerService::handle_read,
                                this,
                                agent_sess,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred))
    );
}

void ControllerService::handle_accept(agent_sess_ptr agent_sess, const boost::system::error_code &error) {
    if (!error) {
        auto agent_remote_address = agent_sess->get_socket().remote_endpoint().address().to_string();
        auto agent_remote_port = agent_sess->get_socket().remote_endpoint().port();
        if (sess_exist(agent_remote_address, agent_remote_port)) {
            LOG_ERROR("Agent already exist, agent: " << agent_remote_address << ":" << agent_remote_port)
            invalid_and_remove_sess(agent_sess);
        } else {
            agent_sess->set_address(agent_remote_address);
            agent_sess->set_port(agent_remote_port);
            sess_lock.lock();
            agent_sessions.push_back(agent_sess);
            sess_lock.unlock();
            LOG_INFO("Receive connection from agent: " << agent_remote_address << ":" << agent_remote_port);

            try {
                controller->associate_sess(agent_sess);
                start_read(agent_sess);
            } catch (...) {
                LOG_ERROR("Associate session Exception!!!")
                invalid_and_remove_sess(agent_sess);
            }
        }
    } else {
        LOG_ERROR("Accept session error: " << error)
        invalid_and_remove_sess(agent_sess);
    }
    start_accept();
}

void ControllerService::handle_read_timeout(agent_sess_ptr agent_sess, boost::system::error_code const &error) {
    if (!error) {
        if (agent_sess->get_read_deadline_timer().expires_at() >= boost::asio::deadline_timer::traits_type::now()) {
            // 计时器时间被重置过,因此没有超时.此超时器被调用是由于其在read函数处理期间排队在队列中

        } else {
            LOG_WARN("Agent read time out")
            // 针对读取超时进行相关操作
            invalid_and_remove_sess(agent_sess);
        }
    } else if (error == boost::asio::error::operation_aborted) {
        // 定时器被重置超时时间
        ;
    } else {
        LOG_ERROR("Read time out error: " << error)
        invalid_and_remove_sess(agent_sess);
    }

}

void ControllerService::handle_write_timeout(agent_sess_ptr agent_sess, boost::system::error_code const &error) {
    if (!error) {
        if (agent_sess->get_write_deadline_timer().expires_at() >= boost::asio::deadline_timer::traits_type::now()) {
            // 计时器时间被重置过,因此没有超时.此超时器被调用是由于其在read函数处理期间排队在队列中

        } else {
            LOG_WARN("Agent write time out")
            // 针对写入超时进行相关操作
            invalid_and_remove_sess(agent_sess);
        }
    } else if (error == boost::asio::error::operation_aborted) {
        // 定时器被重置超时时间
        ;
    } else {
        LOG_ERROR("Write time out error: " << error)
        invalid_and_remove_sess(agent_sess);
    }

}

void ControllerService::handle_write(agent_sess_ptr agent_sess, boost::system::error_code const &error,
                                     size_t bytes_transferred) {
    if (!error) {
        if (!agent_sess->valid()) {
            LOG_WARN("Agent not valid")
            return;
        }

        // 继续发送队列中的消息
        start_write(agent_sess);
    } else if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
        // 对端中断了连接
        invalid_and_remove_sess(agent_sess);
    } else {
        LOG_ERROR("Write data error: " << error)
        invalid_and_remove_sess(agent_sess);
    }
}

void ControllerService::handle_read(agent_sess_ptr agent_sess, boost::system::error_code const &error,
                                    size_t bytes_transferred) {
    if (!error) {
        if (!agent_sess->valid()) {
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
            if (agent_sess->get_cmsg_length() == 0) {
                char ld[msg_length_hdr_size];
                // 该操作会consume read_buf的字节
                agent_sess->get_read_buf().sgetn(ld, msg_length_hdr_size);
                auto dl = get_u32_from_4_u8(uint8_t(ld[0]), uint8_t(ld[1]), uint8_t(ld[2]), uint8_t(ld[3]));
                agent_sess->set_cmsg_length(dl);
            }
            // 根据长度信息判断该报文是否包含了完整的数据部分,如果不是则不处理,否则处理这部分数据
            if (agent_sess->get_read_buf().size() >= agent_sess->get_cmsg_length()) {
                char msg_type_[msg_type_hdr_size];
                agent_sess->get_read_buf().sgetn(msg_type_, msg_type_hdr_size);
                char *msg_body = new char[agent_sess->get_cmsg_length()-msg_type_hdr_size];
                agent_sess->get_read_buf().sgetn(msg_body, agent_sess->get_cmsg_length()-msg_type_hdr_size);
                // 处理消息
                auto msg_type = static_cast<unsigned int>(get_u32_from_2_u8(uint8_t(
                        msg_type_[0]), uint8_t(msg_type_[1])));

                // 交由controller处理消息
                auto msg = std::make_shared<Message>(static_cast<MsgType>(msg_type), msg_body,
                                                     agent_sess->get_cmsg_length()-msg_type_hdr_size);
                try {
                    controller->handle_msg(agent_sess, msg);
                } catch (...) {
                    // controller处理消息异常
                }
                agent_sess->set_cmsg_length(0);

                if (!agent_sess->valid()) {
                    enough_data = false;
                }

                if (agent_sess->get_read_buf().size() < msg_length_hdr_size) {
                    enough_data = false;
                }
            } else {
                enough_data = false;
            }
        }

        // 继续监听
        start_read(agent_sess);
    } else if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
        // 对端中断了连接
        invalid_and_remove_sess(agent_sess);
    } else {
        LOG_ERROR("Read data error: " << error)
        invalid_and_remove_sess(agent_sess);
    }
}

void ControllerService::begin_write(agent_sess_ptr agent_sess) {
    start_write(agent_sess);
}

void ControllerService::start_write(agent_sess_ptr agent_sess) {
    if (!agent_sess->valid()) return;
    msg_ptr msg = nullptr;
    agent_sess->get_msg_lock().lock();
    if (agent_sess->get_messages().empty()) {
        // 没有要发的消息了
        agent_sess->set_sending(false);
        agent_sess->get_msg_lock().unlock();
        agent_sess->get_write_deadline_timer().expires_at(boost::posix_time::pos_infin);
        return;
    }
    msg = agent_sess->get_messages().front();
    agent_sess->get_messages().pop();
    agent_sess->get_msg_lock().unlock();

    // 构建发送的write_buf
    if (agent_sess->get_write_buf().size() != 0) {
        LOG_WARN("Agent write buf is not empty!")
        agent_sess->get_write_buf().consume(agent_sess->get_write_buf().size());
    }

    auto msg_size = get_4_u8_from_u32(static_cast<uint32_t>(msg_type_hdr_size + msg->get_msg_body_size()));
    auto msg_type = get_2_u8_from_u32(static_cast<uint32_t>(msg->get_msg_type()));
    std::ostream os(&(agent_sess->get_write_buf()));
    os.write(msg_size, msg_length_hdr_size);
    os.write(msg_type, msg_type_hdr_size);
    os.write(msg->get_msg_body(), msg->get_msg_body_size());
    delete[] msg_size;
    delete[] msg_type;

    // 设置新的超时器
    agent_sess->get_write_deadline_timer().expires_from_now(
            boost::posix_time::seconds(agent_sess->get_write_timeout()));
    agent_sess->get_write_deadline_timer().async_wait(
            agent_sess->get_strand().wrap(
                    boost::bind(&ControllerService::handle_write_timeout,
                                this, agent_sess,
                                boost::asio::placeholders::error)));
    // 将写入的socket放入poll中
    boost::asio::async_write(
            agent_sess->get_socket(),
            agent_sess->get_write_buf(),
            agent_sess->get_strand().wrap(
                    boost::bind(&ControllerService::handle_write,
                                this,
                                agent_sess,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred))
    );
}