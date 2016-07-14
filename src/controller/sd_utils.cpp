//
// Created by thuanqin on 16/7/14.
//

#include "controller/sd_utils.h"

/*
 * 如果一致则返回false,否则返回true
 * */
bool SDUtils::check_diff(const ogp_msg::ServiceSyncData &src, const ogp_msg::ServiceSyncData &dest) {
    bool result = false;

    if (src.infos().size() != dest.infos().size()) {
        result = true;
        return result;
    }

    for (auto src_info: src.infos()) {
        bool find_this_info = false;
        for (auto dest_info: dest.infos()) {
            if (src_info.service_id() == dest_info.service_id()) {
                find_this_info = true;

                if (src_info.items().size() != dest_info.items().size()) {
                    result = true;
                    return result;
                }

                if (src_info.app_id() != dest_info.app_id()
                        || src_info.app_name() != dest_info.app_name()
                        || src_info.service_port() != dest_info.service_port()
                        || src_info.service_type() != dest_info.service_type()) {
                    result = true;
                    return result;
                }

                for (auto src_item: src_info.items()) {
                    bool find_this_item = false;
                    for (auto dest_item: dest_info.items()) {
                        if (src_item.ma_ip() == dest_item.ma_ip()
                                && src_item.public_port() == dest_item.public_port()) {
                            find_this_item = true;
                            break;
                        }
                    }
                    if (!find_this_item) {
                        result = true;
                        return result;
                    }
                }
            }
        }
        if (!find_this_info) {
            result = true;
            return result;
        }
    }

    return result;
}

