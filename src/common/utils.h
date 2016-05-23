//
// Created by thuanqin on 16/5/13.
//

#ifndef OG_CONTROLLER_UTILS_H
#define OG_CONTROLLER_UTILS_H

#include <cstdint>

uint32_t get_u32_from_4_u8(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
uint32_t get_u32_from_2_u8(uint8_t a, uint8_t b);
char *get_2_u8_from_u32(uint32_t a);
char *get_4_u8_from_u32(uint32_t a);

#endif //OG_CONTROLLER_UTILS_H
