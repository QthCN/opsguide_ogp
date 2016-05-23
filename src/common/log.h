//
// Created by thuanqin on 16/5/14.
//

#ifndef OGP_LOG_H
#define OGP_LOG_H

#include <string>
#include <iostream>
#include <sstream>

class Logger;

extern Logger logger;

enum class Level {
    Debug=0,
    Info=1,
    Warn=2,
    Error=3
};

class Logger {
public:
    Logger() = default;
    Logger(Level level);

    void set_level(Level level);

    void operator()(std::string const& message,
                    char const* function,
                    char const* file,
                    int line,
                    Level level);

private:
    Level level;
};


#define LOG_DEBUG(message) logger(\
                                   static_cast<std::ostringstream&>( \
                                   std::ostringstream().flush() << message \
                                   ).str(),\
                                   __FUNCTION__,\
                                   __FILE__,\
                                   __LINE__, \
                                   Level::Debug);
#define LOG_INFO(message) logger(\
                                   static_cast<std::ostringstream&>( \
                                   std::ostringstream().flush() << message \
                                   ).str(),\
                                   __FUNCTION__,\
                                   __FILE__,\
                                   __LINE__, \
                                   Level::Info);
#define LOG_WARN(message) logger(\
                                   static_cast<std::ostringstream&>( \
                                   std::ostringstream().flush() << message \
                                   ).str(),\
                                   __FUNCTION__,\
                                   __FILE__,\
                                   __LINE__, \
                                   Level::Warn);
#define LOG_ERROR(message) logger(\
                                   static_cast<std::ostringstream&>( \
                                   std::ostringstream().flush() << message \
                                   ).str(),\
                                   __FUNCTION__,\
                                   __FILE__,\
                                   __LINE__, \
                                   Level::Error);

#endif //OGP_LOG_H
