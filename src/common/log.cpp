//
// Created by thuanqin on 16/5/14.
//
#include "common/log.h"

#include <ctime>
#include <sstream>
#include <string>
#include <iostream>

Logger::Logger(Level l) {
    level = l;
}

void Logger::operator()(std::string const &message, char const *function, char const *file, int line, Level level) {
    if (level < this->level) {
        return;
    }
    std::ostringstream os;

    std::string l = "DEBUG";
    switch(level) {
        case Level::Debug:
            l = "DEBUG";
            break;
        case Level::Info:
            l = "INFO";
            break;
        case Level::Warn:
            l = "WARN";
            break;
        case Level::Error:
            l = "ERROR";
            break;
    }

    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];

    time (&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
    std::string dt(buffer);

    os << dt;
    os << " [";
    os << l;
    os << "]";
    os << " [";
    os << file;
    os << "]";
    os << " [";
    os << function;
    os << ":";
    os << line;
    os << "]";
    os << " ";
    os << message;

    auto msg = os.str();
    std::cout << msg << std::endl;
}

void Logger::set_level(Level level) {
    level = level;
}

Logger logger(Level::Info);