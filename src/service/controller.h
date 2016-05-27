//
// Created by thuanqin on 16/5/19.
//

#ifndef OGP_SERVICE_CONTROLLER_H
#define OGP_SERVICE_CONTROLLER_H

#include <queue>
#include <string>
#include <vector>

#include "boost/asio.hpp"
#include "boost/asio/deadline_timer.hpp"

#include "controller/base.h"
#include "service/message.h"
#include "service/session.h"

class ControllerService;

class AgentSession: public Session, public std::enable_shared_from_this<AgentSession> {
public:
    friend class ControllerService;
    AgentSession(boost::asio::io_service &io_service,
                 ControllerService *controller_service,
                 BaseController *controller):
            agent_socket(io_service),
            controller_service(controller_service),
            controller(controller),
            sending(false),
            stopped(false),
            read_deadline_timer(io_service),
            write_deadline_timer(io_service),
            strand(io_service),
            cmsg_length(0){}
    boost::asio::ip::tcp::socket& get_socket() {return agent_socket;};
    AgentSession &set_address(std::string address_) {address = address_; return *this;}
    std::string get_address() const {return address;}
    AgentSession &set_port(unsigned short port_) {port = port_; return *this;}
    unsigned short get_port() const {return port;}
    void send_msg(msg_ptr msg);
    unsigned int get_read_timeout() const {return read_timeout;}
    unsigned int get_write_timeout() const {return write_timeout;}
    AgentSession &set_read_timeout(unsigned int read_timeout_) {read_timeout = read_timeout_; return *this;}
    AgentSession &set_write_timeout(unsigned int write_timeout_) {write_timeout = write_timeout_; return *this;}
    boost::asio::deadline_timer &get_read_deadline_timer() {return read_deadline_timer;}
    boost::asio::deadline_timer &get_write_deadline_timer() {return write_deadline_timer;}
    boost::asio::strand &get_strand() {return strand;}
    boost::asio::streambuf &get_read_buf() {return read_buf;}
    boost::asio::streambuf &get_write_buf() {return write_buf;}
    std::queue<msg_ptr> &get_messages() {return messages;}
    std::mutex &get_msg_lock() {return msg_lock;}
    bool get_sending() {return sending;}
    void set_sending(bool sending_) {sending = sending_;}
    bool valid() const {return not stopped;}
    void stop() {
        stopped = true;
        boost::system::error_code ignored_ec;
        agent_socket.close(ignored_ec);
        read_deadline_timer.cancel();
        write_deadline_timer.cancel();
        while (!messages.empty()) {messages.pop();}
    }
    unsigned int get_cmsg_length() {return cmsg_length;}
    void set_cmsg_length(unsigned int cmsg_length_) {cmsg_length = cmsg_length_;}
    void invalid_sess();

private:
    boost::asio::ip::tcp::socket agent_socket;
    std::string address;
    unsigned short port;
    ControllerService *controller_service;
    BaseController *controller;
    // 等待发送给agent的消息队列
    std::queue<msg_ptr> messages;
    // 是否在发送消息中
    bool sending;
    // 发送操作的互斥锁
    std::mutex msg_lock;
    // 读取操作的超时触发器,超时时间目前等于心跳同步时间
    boost::asio::deadline_timer read_deadline_timer;
    // 发送操作的超时触发器
    boost::asio::deadline_timer write_deadline_timer;
    // session的strand
    boost::asio::strand strand;
    // 超时时间
    unsigned int read_timeout = 30;
    unsigned int write_timeout = 30;
    boost::asio::streambuf read_buf;
    boost::asio::streambuf write_buf;
    bool stopped = false;
    // 当前处理的消息的消息体长度
    unsigned int cmsg_length;
};

class ControllerService {
public:
    typedef std::shared_ptr<AgentSession> agent_sess_ptr;

    ControllerService(unsigned int thread_num, const std::string &listen_address,
                      unsigned int listen_port, BaseController *controller);
    void run();
    void begin_write(agent_sess_ptr agent_sess);

private:
    void invalid_and_remove_sess(agent_sess_ptr agent_sess);
    bool sess_exist(agent_sess_ptr agent_sess);
    bool sess_exist(const std::string &address, unsigned short port);
    void start_accept();
    void start_read(agent_sess_ptr agent_sess);
    void start_write(agent_sess_ptr agent_sess);
    void handle_accept(agent_sess_ptr agent_sess, const boost::system::error_code& error);
    void handle_read_timeout(agent_sess_ptr agent_sess, boost::system::error_code const &error);
    void handle_write_timeout(agent_sess_ptr agent_sess, boost::system::error_code const &error);
    void handle_read(agent_sess_ptr agent_sess, boost::system::error_code const &error, size_t bytes_transferred);
    void handle_write(agent_sess_ptr agent_sess, boost::system::error_code const &error, size_t bytes_transferred);
    size_t read_completion_handler(agent_sess_ptr agent_sess,
                                   boost::system::error_code const &error,
                                   size_t bytes_transferred);

    BaseController *controller;
    unsigned int thread_num;
    std::string listen_address;
    unsigned int listen_port;
    std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor;
    boost::asio::io_service io_service;
    std::vector<agent_sess_ptr> agent_sessions;
    std::mutex sess_lock;
};

#endif //OGP_SERVICE_CONTROLLER_H
