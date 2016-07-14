//
// Created by thuanqin on 16/7/14.
//

#ifndef OGP_CONTROLLER_SD_UTILS_H
#define OGP_CONTROLLER_SD_UTILS_H

#include "ogp_msg.pb.h"

class SDUtils {
public:
    bool check_diff(const ogp_msg::ServiceSyncData& src, const ogp_msg::ServiceSyncData& dest);
};

#endif //OGP_CONTROLLER_SD_UTILS_H
