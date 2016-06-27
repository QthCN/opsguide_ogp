//
// Created by thuanqin on 16/5/24.
//

#ifndef OGP_SERVICE_AGENT_H
#define OGP_SERVICE_AGENT_H

#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "boost/asio.hpp"
#include "boost/asio/deadline_timer.hpp"

#include "controller/base.h"
#include "service/message.h"
#include "service/session.h"

class AgentService;

class ControllerSession: public Session, public std::enable_shared_from_this<ControllerSession> {
public:
    friend class AgentService;
    ControllerSession(boost::asio::io_service &io_service,
                      AgentService *agent_service,
                 BaseController *controller):
            agent_socket(io_service),
            agent_service(agent_service),
            controller(controller),
            sending(false),
            stopped(false),
            read_deadline_timer(io_service),
            write_deadline_timer(io_service),
            strand(io_service),
            cmsg_length(0){}
    boost::asio::ip::tcp::socket& get_socket() {return agent_socket;};
    ControllerSession &set_address(std::string address_) {address = address_; return *this;}
    std::string get_address() const {return address;}
    ControllerSession &set_port(unsigned short port_) {port = port_; return *this;}
    unsigned short get_port() const {return port;}
    void send_msg(msg_ptr msg);
    unsigned int get_read_timeout() const {return read_timeout;}
    unsigned int get_write_timeout() const {return write_timeout;}
    ControllerSession &set_read_timeout(unsigned int read_timeout_) {read_timeout = read_timeout_; return *this;}
    ControllerSession &set_write_timeout(unsigned int write_timeout_) {write_timeout = write_timeout_; return *this;}
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
        read_deadline_timer.cancel(ignored_ec);
        write_deadline_timer.cancel(ignored_ec);
        while (!messages.empty()) {messages.pop();}
    }
    unsigned int get_cmsg_length() {return cmsg_length;}
    void set_cmsg_length(unsigned int cmsg_length_) {cmsg_length = cmsg_length_;}
    void invalid_sess();

private:
    boost::asio::ip::tcp::socket agent_socket;
    std::string address;
    unsigned short port;
    AgentService *agent_service;
    BaseController *controller;
    // 等待发送给controller的消息队列
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

class AgentService {
public:
    typedef std::shared_ptr<ControllerSession> controller_sess_ptr;

    AgentService(unsigned int thread_num, const std::string &controller_address,
                      unsigned int controller_port, BaseController *controller);
    void run();
    void begin_write(controller_sess_ptr controller_sess);

private:
    void invalid_and_remove_sess(controller_sess_ptr controller_sess);
    void start_connect();
    void reconnect();
    void start_read(controller_sess_ptr controller_sess);
    void start_write(controller_sess_ptr controller_sess);
    void handle_read_timeout(controller_sess_ptr controller_sess, boost::system::error_code const &error);
    void handle_write_timeout(controller_sess_ptr controller_sess, boost::system::error_code const &error);
    void handle_connect(controller_sess_ptr controller_sess, boost::system::error_code);
    void handle_read(controller_sess_ptr controller_sess, boost::system::error_code const &error, size_t bytes_transferred);
    void handle_write(controller_sess_ptr controller_sess, boost::system::error_code const &error, size_t bytes_transferred);
    size_t read_completion_handler(controller_sess_ptr controller_sess,
                                   boost::system::error_code const &error,
                                   size_t bytes_transferred);

    BaseController *controller;
    unsigned int thread_num;
    std::string controller_address;
    unsigned int controller_port;
    boost::asio::io_service io_service;
    std::vector<controller_sess_ptr> controller_sessions;
    std::mutex sess_lock;
};

#endif //OGP_SERVICE_AGENT_H
