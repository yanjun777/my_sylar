#include "../sylar/log.h"
#include <iostream> 


int main() {
    
    sylar::Logger::ptr logger(new sylar::Logger);
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));

    sylar::LogEvent::ptr event(new sylar::LogEvent(__FILE__, __LINE__, 0, 0, 0, 0));

    logger->log(sylar::LogLevel::DEBUG, event);
    std::cout << "hello sylar" << std::endl;

    return 0; 
}