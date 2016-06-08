//
// Created by thuanqin on 16/5/13.
//
#include "common/utils.h"

uint32_t get_u32_from_4_u8(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    uint32_t v = 0;
    v = (v | a << 24 | b << 16 | c << 8 | d);
    return v;
}

uint32_t get_u32_from_2_u8(uint8_t a, uint8_t b) {
    uint32_t v = 0;
    v = (v | a << 8 | b);
    return v;
}

char *get_2_u8_from_u32(uint32_t a) {
    char *r = new char[2];
    r[0] = static_cast<uint8_t>(a >> 8 & 255);
    r[1] = static_cast<uint8_t>(a & 255);
    return r;
}

char *get_4_u8_from_u32(uint32_t a) {
    char *r = new char[4];
    r[0] = static_cast<uint8_t>(a >> 24 & 255);
    r[1] = static_cast<uint8_t>(a >> 16 & 255);
    r[2] = static_cast<uint8_t>(a >> 8 & 255);
    r[3] = static_cast<uint8_t>(a & 255);
    return r;
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}