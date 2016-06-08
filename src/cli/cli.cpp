//
// Created by thuanqin on 16/6/7.
//
#include <iostream>
#include <string>

#include "boost/asio.hpp"
#include "boost/optional.hpp"
#include "boost/program_options.hpp"

#include "common/log.h"
#include "common/utils.h"
#include "ogp_msg.pb.h"
#include "service/message.h"

using namespace boost::program_options;

options_description desc("Allowed options");
variables_map vm;

// based on http://stackoverflow.com/questions/13126776/asioread-with-timeout
void readWithTimeout(boost::asio::ip::tcp::socket& s, boost::asio::streambuf &streambuf,
                     const boost::asio::deadline_timer::duration_type& expiry_time)
{
    boost::optional<boost::system::error_code> timer_result;
    boost::asio::deadline_timer timer(s.get_io_service());
    timer.expires_from_now(expiry_time);
    timer.async_wait([&timer_result] (const boost::system::error_code& error) { timer_result.reset(error); });

    boost::optional<boost::system::error_code> read_result;
    boost::asio::async_read(s, streambuf,
                            [&read_result] (const boost::system::error_code& error, size_t) {
                                read_result.reset(error); });

    s.get_io_service().reset();
    while (s.get_io_service().run_one())
    {
        if (read_result) {
            timer.cancel();
            break;
        }
        else if (timer_result)
            break;
    }
}

template<typename PROTOBUF_MSG>
static void send_msg(MsgType msg_type,
              PROTOBUF_MSG *protobuf_msg=nullptr, bool need_reply=true,
              MsgType *res_msg_type_=nullptr, char **res_msg_body_=nullptr, int *res_msg_body_size=nullptr) {
    const int send_msg_time_out = 10; // 10秒
    auto send_msg_begin_time = std::time(0);
    auto controller_full_address = vm["controller-address"].as<std::string>();
    auto controller_ip = split(controller_full_address, ':')[0];
    auto controller_port = atoi(split(controller_full_address, ':')[1].c_str());
    LOG_INFO("send request to: " << controller_ip  << ":" << std::to_string(controller_port))

    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket sock(io_service);
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(controller_ip), controller_port);
    sock.connect(ep);

    boost::asio::streambuf write_buf;
    boost::asio::streambuf read_buf;

    // 发送请求
    int msg_size = 0;
    char *msg_data = nullptr;
    if (protobuf_msg != nullptr) {
        msg_size = protobuf_msg->ByteSize();
        msg_data = new char[msg_size];
        protobuf_msg->SerializeToArray(msg_data, msg_size);
    }

    std::ostream os(&write_buf);
    auto raw_msg_size = get_4_u8_from_u32(static_cast<uint32_t>(msg_type_hdr_size + msg_size));
    auto raw_msg_type = get_2_u8_from_u32(static_cast<uint32_t>(msg_type));
    os.write(raw_msg_size, msg_length_hdr_size);
    os.write(raw_msg_type, msg_type_hdr_size);
    if (protobuf_msg != nullptr) {
        os.write(msg_data, msg_size);
        delete[] msg_data;
    }
    int msg_send = 0;
    while (msg_send < (msg_length_hdr_size + msg_type_hdr_size + msg_size)) {
        msg_send += boost::asio::write(sock, write_buf);
        if (std::time(0) - send_msg_begin_time > send_msg_time_out) {
            LOG_ERROR("send message timeout.")
            throw std::runtime_error("send message timeout.");
        }
    }
    delete[] raw_msg_size;
    delete[] raw_msg_type;

    if (!need_reply) {
        sock.close();
        return;
    }

    // 接收消息
    bool size_handled = false;
    int res_msg_size = 0;

    sock.native_non_blocking(true);
    while (true) {
        if (std::time(0) - send_msg_begin_time > send_msg_time_out) {
            LOG_ERROR("receive message timeout.")
            throw std::runtime_error("receive message timeout.");
        }

        readWithTimeout(sock, read_buf, boost::posix_time::seconds(0));
        // 长度头部没有接收完
        if (read_buf.size() <= msg_length_hdr_size) continue;

        // 长度头部接收完了,解析长度
        if (!size_handled) {
            char ld[msg_length_hdr_size];
            read_buf.sgetn(ld, msg_length_hdr_size);
            res_msg_size = get_u32_from_4_u8(uint8_t(ld[0]), uint8_t(ld[1]), uint8_t(ld[2]), uint8_t(ld[3]));
            size_handled = true;
        }

        // 根据头部长度判断整个报文是否接收完
        if (size_handled and read_buf.size() < res_msg_size) continue;

        // 已经获取到了完整的报文,进行相关信息的解析
        char msg_type_[msg_type_hdr_size];
        read_buf.sgetn(msg_type_, msg_type_hdr_size);
        char *res_msg_body = new char[res_msg_size - msg_type_hdr_size];
        read_buf.sgetn(res_msg_body, res_msg_size - msg_type_hdr_size);
        *res_msg_body_size = res_msg_size - msg_type_hdr_size;
        auto res_msg_type = static_cast<MsgType>(static_cast<unsigned int>(get_u32_from_2_u8(uint8_t(
                msg_type_[0]), uint8_t(msg_type_[1]))));
        *res_msg_type_ = res_msg_type;
        *res_msg_body_ = res_msg_body;
        sock.close();
        break;
    }
}

static int add_application(std::string app_name, std::string app_version, std::string app_desc,
             std::string app_source, std::string app_version_desc) {
    ogp_msg::AddApplicationReq add_app_req;
    add_app_req.set_app_name(app_name);
    add_app_req.set_app_version(app_version);
    add_app_req.set_app_desc(app_desc);
    add_app_req.set_app_source(app_source);
    add_app_req.set_app_version_desc(app_version_desc);
    MsgType res_msg_type;
    char *res_msg_body;
    int res_msg_size;

    send_msg(MsgType::CI_CLI_ADD_APP_REQ, &add_app_req, true, &res_msg_type, &res_msg_body, &res_msg_size);

    ogp_msg::AddApplicationRes add_app_res;
    add_app_res.ParseFromArray(res_msg_body, res_msg_size);
    delete[] res_msg_body;

    if (add_app_res.header().rc()) {
        LOG_ERROR("" << add_app_res.header().message())
        return -1;
    }
    LOG_INFO("add app & version successful.")
    return 0;
}

int main(int argc, char *argv[]) {
    // 构造参数
    desc.add_options()
            // help
            ("help,h", "produce help message")

            // controller
            ("controller-address", value<std::string>()->default_value("127.0.0.1:9900"), "controller-address")

            // add-app
            ("add-app", value<bool>()->default_value(false), "add app")
            ("app-name", value<std::string>()->default_value(""), "app name[need --add-app]")
            ("app-version", value<std::string>()->default_value(""), "app version[need --add-app]")
            ("app-desc", value<std::string>()->default_value(""), "app description[need --add-app]")
            ("app-source", value<std::string>()->default_value(""), "app source[need --add-app]")
            ("app-version-desc", value<std::string>()->default_value(""), "app version description[need --add-app]")
            ;

    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

    // 根据请求进行相关处理
    // 新增APP
    auto add_app = vm["add-app"].as<bool>();
    if (add_app) {
        auto app_name = vm["app-name"].as<std::string>();
        auto app_version = vm["app-version"].as<std::string>();
        auto app_desc = vm["app-desc"].as<std::string>();
        auto app_source = vm["app-source"].as<std::string>();
        auto app_version_desc = vm["app-version-desc"].as<std::string>();

        if (app_name == ""
            || app_version == ""
            || app_source == ""
            || app_version_desc == "") {
            std::cout << "请提供app的足够信息" << std::endl;
            exit(-1);
        }
        exit(add_application(app_name, app_version, app_desc, app_source, app_version_desc));
    } // else if {} ...
}

