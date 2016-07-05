//
// Created by thuanqin on 16/7/5.
//
#include "service/proxy.h"

#include <cstdint>
#include <thread>
#include <vector>

#include "boost/asio.hpp"
#include "boost/bind.hpp"

#include "common/config.h"
#include "common/log.h"
#include "common/utils.h"

void ProxySession::send_msg(msg_ptr msg) {
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
    proxy_service->begin_write(shared_from_this());
}

void ProxySession::invalid_sess() {
    if (!valid()) return;
    stop();
}

ProxyService::ProxyService(unsigned int s_thread_num,
                           unsigned int c_thread_num,
                           const std::string &controller_address,
                           unsigned int controller_port,
                           const std::string &listen_address,
                           unsigned int listen_port,
                           BaseController *controller,
                           MsgType srv_msg_id_type):s_thread_num(s_thread_num),
                                                       c_thread_num(c_thread_num),
                                                       listen_address(listen_address),
                                                       listen_port(listen_port),
                                                       controller_address(controller_address),
                                                       controller_port(controller_port),
                                                       controller(controller),
                                                       srv_msg_id_type(srv_msg_id_type){
    controller->init();

    start_connect();

    if (listen_address == "0.0.0.0") {
        auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), listen_port);
        acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(c_io_service, endpoint);
    } else {
        auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(listen_address), listen_port);
        acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(c_io_service, endpoint);
    }
    start_accept();
}

bool ProxyService::sess_exist(proxy_sess_ptr proxy_sess) {
    sess_lock.lock();
    auto ret = false;
    auto ip_address = proxy_sess->get_address();
    auto port = proxy_sess->get_port();
    for (auto s: proxy_sessions) {
        if (s->get_address()==ip_address && s->get_port()==port) {
            ret = true;
            break;
        }
    }
    sess_lock.unlock();
    return ret;
}

bool ProxyService::sess_exist(const std::string &address, unsigned short port) {
    sess_lock.lock();
    auto ret = false;
    for (auto s: proxy_sessions) {
        if (s->get_address()==address && s->get_port()==port) {
            ret = true;
            break;
        }
    }
    sess_lock.unlock();
    return ret;
}

void ProxyService::handle_accept(proxy_sess_ptr proxy_sess, const boost::system::error_code &error) {
    if (!error) {
        auto client_remote_address = proxy_sess->get_socket().remote_endpoint().address().to_string();
        auto client_remote_port = proxy_sess->get_socket().remote_endpoint().port();
        if (sess_exist(client_remote_address, client_remote_port)) {
            LOG_ERROR("Client already exist, info: " << client_remote_address << ":" << client_remote_port)
            invalid_and_remove_sess(proxy_sess);
        } else {
            proxy_sess->set_address(client_remote_address);
            proxy_sess->set_port(client_remote_port);
            sess_lock.lock();
            proxy_sessions.push_back(proxy_sess);
            sess_lock.unlock();
            LOG_INFO("Receive connection from client: " << client_remote_address << ":" << client_remote_port);

            try {
                controller->associate_sess(proxy_sess);
                start_read(proxy_sess);
            } catch (...) {
                LOG_ERROR("Associate session Exception!!!")
                invalid_and_remove_sess(proxy_sess);
            }
        }
    } else {
        LOG_ERROR("Accept session error: " << error)
        invalid_and_remove_sess(proxy_sess);
    }
    start_accept();
}

void ProxyService::start_accept() {
    proxy_sess_ptr proxy_sess = std::make_shared<ProxySession>(c_io_service, this, controller, T_PS_CLIENT);
    proxy_sess->set_read_timeout(static_cast<unsigned int>(config_mgr.get_item("client_read_timeout")->get_int()));
    proxy_sess->set_write_timeout(static_cast<unsigned int>(config_mgr.get_item("client_write_timeout")->get_int()));
    acceptor->async_accept(proxy_sess->get_socket(),
                           boost::bind(&ProxyService::handle_accept, this,
                                       proxy_sess, boost::asio::placeholders::error));
}

void ProxyService::invalid_and_remove_sess(proxy_sess_ptr proxy_sess) {
    sess_lock.lock();
    if (proxy_sess->valid()) {
        // 通知controller移除此session相关的信息
        controller->invalid_sess(proxy_sess);
        // 通知session清理相关资源
        proxy_sess->invalid_sess();
    }
    // 删除对session的引用
    for (auto k=proxy_sessions.begin(); k!=proxy_sessions.end(); k++) {
        if ((*k)->get_address() == proxy_sess->get_address()
            && (*k)->get_port() == proxy_sess->get_port()) {
            proxy_sessions.erase(k);
            break;
        }
    }
    sess_lock.unlock();
}

void ProxyService::handle_connect(proxy_sess_ptr proxy_sess, boost::system::error_code error) {
    if (!error) {
        auto controller_remote_address = proxy_sess->get_socket().remote_endpoint().address().to_string();
        auto controller_remote_port = proxy_sess->get_socket().remote_endpoint().port();
        proxy_sess->set_address(controller_remote_address);
        proxy_sess->set_port(controller_remote_port);
        LOG_INFO("Connected to controller: " << controller_remote_address << ":" << controller_remote_port);

        try {
            controller->associate_sess(proxy_sess);
            // 通知Controller我是什么类型
            proxy_sess->send_msg(std::make_shared<Message>(
                    srv_msg_id_type,
                    new char[0],
                    0
            ));
        } catch (...) {
            LOG_ERROR("Session association Exception!!!")
            invalid_and_remove_sess(proxy_sess);
            reconnect();
        }
        start_read(proxy_sess);
    } else {
        LOG_ERROR("Connect to controller error: " << error)
        invalid_and_remove_sess(proxy_sess);
        reconnect();
    }
}

void ProxyService::reconnect() {
    sess_lock.lock();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    if (proxy_sessions.size() == 0) {
        LOG_INFO("Reconnect to controller now.")
        start_connect();
    }
    sess_lock.unlock();
}

void ProxyService::start_connect() {
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(controller_address), controller_port);
    proxy_sess_ptr proxy_sess = std::make_shared<ProxySession>(s_io_service, this, controller, T_PS_SERVER);
    proxy_sess->set_read_timeout(static_cast<unsigned int>(config_mgr.get_item("server_read_timeout")->get_int()));
    proxy_sess->set_write_timeout(static_cast<unsigned int>(config_mgr.get_item("server_write_timeout")->get_int()));
    proxy_sessions.push_back(proxy_sess);
    boost::asio::ip::tcp::endpoint cep(boost::asio::ip::address::from_string(
            config_mgr.get_item("proxy_bind_address")->get_str()), 0);
    proxy_sess->get_socket().open(boost::asio::ip::tcp::v4());
    proxy_sess->get_socket().bind(cep);
    proxy_sess->get_socket().async_connect(ep,
                                           boost::bind(&ProxyService::handle_connect, this,
                                                       proxy_sess, boost::asio::placeholders::error));
}

void ProxyService::run() {
    LOG_INFO("ProxyService run. Server thread num: " << s_thread_num
             << " Client thread num: " << c_thread_num
             << " Controller Address: " << controller_address << " Port: " << controller_port
             << " Listen Address: " << listen_address << " Listen Port: " << listen_port)
    // 监听线程
    for (unsigned int i=0; i<s_thread_num; i++) {
        std::thread t([this](){this->s_io_service.run();});
        controller->add_thread(std::move(t));
    }
    for (unsigned int i=0; i<c_thread_num; i++) {
        std::thread t([this](){this->c_io_service.run();});
        controller->add_thread(std::move(t));
    }
    controller->wait();
}


size_t ProxyService::read_completion_handler(proxy_sess_ptr proxy_sess, boost::system::error_code const &error,
                                             size_t bytes_transferred) {
    if (!error) {
        if (proxy_sess->get_cmsg_length() == 0) {
            if (proxy_sess->get_read_buf().size() >= msg_length_hdr_size) {
                return 0;
            }
        } else {
            if (proxy_sess->get_read_buf().size() >= proxy_sess->get_cmsg_length()) {
                return 0;
            }
        }
        return 1024;
    } else if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
        // 对端中断了连接
        invalid_and_remove_sess(proxy_sess);
        if (proxy_sess->get_sess_type() == T_PS_SERVER) {
            reconnect();
        }
        return 0;
    } else {
        LOG_ERROR("read_completion_handler error: " << error)
        invalid_and_remove_sess(proxy_sess);
        if (proxy_sess->get_sess_type() == T_PS_SERVER) {
            reconnect();
        }
        return 0;
    }
}

void ProxyService::start_read(proxy_sess_ptr proxy_sess) {
    if (!proxy_sess->valid()) return;
    // 设置新的超时器
    proxy_sess->get_read_deadline_timer().expires_from_now(
            boost::posix_time::seconds(proxy_sess->get_read_timeout()));
    proxy_sess->get_read_deadline_timer().async_wait(
            proxy_sess->get_strand().wrap(
                    boost::bind(&ProxyService::handle_read_timeout,
                                this, proxy_sess,
                                boost::asio::placeholders::error)));
    // 将读取的socket放入poll中
    boost::asio::async_read(
            proxy_sess->get_socket(),
            proxy_sess->get_read_buf(),
            boost::bind(&ProxyService::read_completion_handler,
                        this,
                        proxy_sess,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred),
            proxy_sess->get_strand().wrap(
                    boost::bind(&ProxyService::handle_read,
                                this,
                                proxy_sess,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred))
    );
}

void ProxyService::handle_read_timeout(proxy_sess_ptr proxy_sess, boost::system::error_code const &error) {
    if (!error) {
        if (proxy_sess->get_read_deadline_timer().expires_at() >= boost::asio::deadline_timer::traits_type::now()) {
            // 计时器时间被重置过,因此没有超时.此超时器被调用是由于其在read函数处理期间排队在队列中

        } else {
            LOG_WARN("Read time out")
            // 针对读取超时进行相关操作
            invalid_and_remove_sess(proxy_sess);
            if (proxy_sess->get_sess_type() == T_PS_SERVER) {
                reconnect();
            }
        }
    } else if (error == boost::asio::error::operation_aborted) {
        // 定时器被重置超时时间
        ;
    } else {
        LOG_ERROR("Read time out error: " << error)
        invalid_and_remove_sess(proxy_sess);
        if (proxy_sess->get_sess_type() == T_PS_SERVER) {
            reconnect();
        }
    }

}

void ProxyService::handle_write_timeout(proxy_sess_ptr proxy_sess, boost::system::error_code const &error) {
    if (!error) {
        if (proxy_sess->get_write_deadline_timer().expires_at() >= boost::asio::deadline_timer::traits_type::now()) {
            // 计时器时间被重置过,因此没有超时.此超时器被调用是由于其在read函数处理期间排队在队列中

        } else {
            LOG_WARN("Session write time out")
            // 针对写入超时进行相关操作
            invalid_and_remove_sess(proxy_sess);
            if (proxy_sess->get_sess_type() == T_PS_SERVER) {
                reconnect();
            }
        }
    } else if (error == boost::asio::error::operation_aborted) {
        // 定时器被重置超时时间
        ;
    } else {
        LOG_ERROR("Write time out error: " << error)
        // 如果必要要对socket进行关闭操作,否则socket会一直在poll中write
        invalid_and_remove_sess(proxy_sess);
        if (proxy_sess->get_sess_type() == T_PS_SERVER) {
            reconnect();
        }
    }

}

void ProxyService::handle_write(proxy_sess_ptr proxy_sess, boost::system::error_code const &error,
                                size_t bytes_transferred) {
    if (!error) {
        if (!proxy_sess->valid()) {
            LOG_WARN("Session not valid")
            return;
        }

        // 继续发送队列中的消息
        start_write(proxy_sess);
    } else if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
        // 对端中断了连接
        invalid_and_remove_sess(proxy_sess);
        if (proxy_sess->get_sess_type() == T_PS_SERVER) {
            reconnect();
        }
    } else {
        LOG_ERROR("Write data error: " << error)
        invalid_and_remove_sess(proxy_sess);
        if (proxy_sess->get_sess_type() == T_PS_SERVER) {
            reconnect();
        }
    }
}

void ProxyService::handle_read(proxy_sess_ptr proxy_sess, boost::system::error_code const &error,
                               size_t bytes_transferred) {
    if (!error) {
        if (!proxy_sess->valid()) {
            LOG_WARN("Session not valid")
            return;
        }

        bool enough_data = true;
        while (enough_data) {
            /*
             * 报文格式为: 4字节余下部分总长度 + 2字节msg类型 + protobuf报文
             * 例如收到一个10字节的报文,则开始的4个字节为0x00000006,接着是2字节的类型,如0x0001,在接着是4字节的profobuf内容
             * */

            // 获取长度信息
            if (proxy_sess->get_cmsg_length() == 0) {
                char ld[msg_length_hdr_size];
                // 该操作会consume read_buf的字节
                proxy_sess->get_read_buf().sgetn(ld, msg_length_hdr_size);
                auto dl = get_u32_from_4_u8(uint8_t(ld[0]), uint8_t(ld[1]), uint8_t(ld[2]), uint8_t(ld[3]));
                proxy_sess->set_cmsg_length(dl);
            }
            // 根据长度信息判断该报文是否包含了完整的数据部分,如果不是则不处理,否则处理这部分数据
            if (proxy_sess->get_read_buf().size() >= proxy_sess->get_cmsg_length()) {
                char msg_type_[msg_type_hdr_size];
                proxy_sess->get_read_buf().sgetn(msg_type_, msg_type_hdr_size);
                char *msg_body = new char[proxy_sess->get_cmsg_length()-msg_type_hdr_size];
                proxy_sess->get_read_buf().sgetn(msg_body, proxy_sess->get_cmsg_length()-msg_type_hdr_size);
                // 处理消息
                auto msg_type = static_cast<unsigned int>(get_u32_from_2_u8(uint8_t(
                        msg_type_[0]), uint8_t(msg_type_[1])));

                // 交由controller处理消息
                auto msg = std::make_shared<Message>(static_cast<MsgType>(msg_type), msg_body,
                                                     proxy_sess->get_cmsg_length()-msg_type_hdr_size);
                try {
                    controller->handle_msg(proxy_sess, msg);
                } catch (...) {
                    // controller处理消息异常
                }
                proxy_sess->set_cmsg_length(0);

                if (!proxy_sess->valid()) {
                    enough_data = false;
                }

                if (proxy_sess->get_read_buf().size() < msg_length_hdr_size) {
                    enough_data = false;
                }
            } else {
                enough_data = false;
            }
        }

        // 继续监听
        start_read(proxy_sess);
    } else if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
        // 对端中断了连接
        invalid_and_remove_sess(proxy_sess);
        if (proxy_sess->get_sess_type() == T_PS_SERVER) {
            reconnect();
        }
    } else {
        LOG_ERROR("Read data error: " << error)
        invalid_and_remove_sess(proxy_sess);
        if (proxy_sess->get_sess_type() == T_PS_SERVER) {
            reconnect();
        }
    }
}

void ProxyService::begin_write(proxy_sess_ptr proxy_sess) {
    start_write(proxy_sess);
}

void ProxyService::start_write(proxy_sess_ptr proxy_sess) {
    if (!proxy_sess->valid()) return;
    msg_ptr msg = nullptr;
    proxy_sess->get_msg_lock().lock();
    if (proxy_sess->get_messages().empty()) {
        // 没有要发的消息了
        proxy_sess->set_sending(false);
        proxy_sess->get_msg_lock().unlock();
        proxy_sess->get_write_deadline_timer().expires_at(boost::posix_time::pos_infin);
        return;
    }
    msg = proxy_sess->get_messages().front();
    proxy_sess->get_messages().pop();
    proxy_sess->get_msg_lock().unlock();

    // 构建发送的write_buf
    if (proxy_sess->get_write_buf().size() != 0) {
        LOG_WARN("Write buf is not empty!")
        proxy_sess->get_write_buf().consume(proxy_sess->get_write_buf().size());
    }

    auto msg_size = get_4_u8_from_u32(static_cast<uint32_t>(msg_type_hdr_size + msg->get_msg_body_size()));
    auto msg_type = get_2_u8_from_u32(static_cast<uint32_t>(msg->get_msg_type()));
    std::ostream os(&(proxy_sess->get_write_buf()));
    os.write(msg_size, msg_length_hdr_size);
    os.write(msg_type, msg_type_hdr_size);
    os.write(msg->get_msg_body(), msg->get_msg_body_size());
    delete[] msg_size;
    delete[] msg_type;

    // 设置新的超时器
    proxy_sess->get_write_deadline_timer().expires_from_now(
            boost::posix_time::seconds(proxy_sess->get_write_timeout()));
    proxy_sess->get_write_deadline_timer().async_wait(
            proxy_sess->get_strand().wrap(
                    boost::bind(&ProxyService::handle_write_timeout,
                                this, proxy_sess,
                                boost::asio::placeholders::error)));
    // 将写入的socket放入poll中
    boost::asio::async_write(
            proxy_sess->get_socket(),
            proxy_sess->get_write_buf(),
            proxy_sess->get_strand().wrap(
                    boost::bind(&ProxyService::handle_write,
                                this,
                                proxy_sess,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred))
    );
}
