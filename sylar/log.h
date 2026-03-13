#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <string>
#include <stdint.h> // 定义了int32_t和uint32_t 
#include <memory>
#include <list> 
#include <fstream>
#include <vector> 
#include <sstream> 

namespace sylar{

//日志事件
class Logger; 
class LogEvent{
public:
    LogEvent();
    typedef std::shared_ptr<LogEvent> ptr; 
    LogEvent(const char* file, int32_t line, int32_t elapse
        , uint32_t threadId, uint32_t fiberId, uint64_t time);
    const char* getFile() const { return m_file; }
    int32_t getLine() const { return m_line; }
    int32_t getElapse() const { return m_elapse; }
    uint32_t getThreadId() const { return m_threadId; }
    uint32_t getFiberId() const { return m_fiberId; }
    uint64_t getTime() const { return m_time; }
    const std::string& getContent() const { return m_content; } 
    std::stringstream& getSS() { return m_ss; } 

private:
    const char* m_file = nullptr;
    int32_t m_line = 0; //行号
    int32_t m_elapse = 0; //程序启动到现在的毫秒数
    uint32_t m_threadId = 0; //线程id
    uint32_t m_fiberId = 0; //协程id
    uint64_t m_time = 0; //时间
    std::string m_content; //日志内容
    std::stringstream m_ss; //日志内容  
};

// 级别
class LogLevel{
public:
    enum Level{
        UNKNOW = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    }; 
    static const char* toString(Level level);
}; 

// 前向声明    LogFormatter 需要用到Logger 
class Logger; 

//日志格式器
class LogFormatter{
public:
    LogFormatter(const std::string& pattern);
    typedef std::shared_ptr<LogFormatter> ptr; 
    // %t    %thread_id %m %n  
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    void init();   
public:
    class FormatItem{
    public: 
        typedef std::shared_ptr<FormatItem> ptr;  
        virtual ~FormatItem() {};
        // 纯虚函数 子类必须实现 这里用于格式化日志内容  
        virtual void format(std::ostream& os,std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };
private: 
    std::string m_pattern; //日志格式  
    std::vector<FormatItem::ptr> m_items; //格式项   
    
};

//日志输出地
class LogAppender{
public:
    typedef std::shared_ptr<LogAppender> ptr;  
    virtual ~LogAppender(); 
    //纯虚函数 子类必须实现   
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0 ;   //日志输出

    void setFormatter(LogFormatter::ptr formatter){ m_formatter = formatter; }  //设置日志格式器  
    LogFormatter::ptr getFormatter() const { return m_formatter; }  //获取日志格式器   
protected:
    LogLevel::Level m_level; //日志级别
    LogFormatter::ptr m_formatter; //日志格式器 
};

// 输出到标准输出
class StdoutLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr; 
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;  
};

// 输出到文件
class FileLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr; 
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;  
    FileLogAppender(const std::string& filename);

    bool reopen(); //重新打开文件   
    
private:
    std::string m_filename; //文件名  
    std::ofstream m_filestream; //文件流   
};


//日志器 
class Logger : public std::enable_shared_from_this<Logger>{
public:
    typedef std::shared_ptr<Logger> ptr;  

    Logger(const std::string& name = "root");
    void log(LogLevel::Level level, LogEvent::ptr event); //日志输出

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender); 
    void delAppender(LogAppender::ptr appender);  
    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level level) { m_level = level; }
    std::string getName() const { return m_name; }
    
private:
    std::string m_name; //日志器名称 
    LogLevel::Level m_level; //日志级别 
    std::list<LogAppender::ptr> m_appenders; //日志输出地 
    LogFormatter::ptr m_formatter; //日志格式器  
};






}


#endif