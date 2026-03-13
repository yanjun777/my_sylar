#include "log.h"

#include <cctype>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <tuple>

namespace sylar {

namespace {

typedef LogFormatter::FormatItem::ptr FormatItemPtr;
typedef std::function<FormatItemPtr(const std::string&)> FormatItemCreator;

class MessageFormatItem : public LogFormatter::FormatItem {
public:
    explicit MessageFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    explicit LevelFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << LogLevel::toString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    explicit ElapseFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
    explicit NameFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << logger->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    explicit ThreadIdFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    explicit FiberIdFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    explicit DateTimeFormatItem(const std::string& fmt = "%Y-%m-%d %H:%M:%S")
        : m_format(fmt) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getTime();
    }
private:
    std::string m_format;
};

class FileNameFormatItem : public LogFormatter::FormatItem {
public:
    explicit FileNameFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    explicit LineFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    explicit NewLineFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    explicit StringFormatItem(const std::string& str)
        : m_str(str) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << m_str;
    }
private:
    std::string m_str;
};

} // namespace

// ==================== LogLevel ====================

const char* LogLevel::toString(LogLevel::Level level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOW";
    }
}

// ==================== LogEvent ====================

LogEvent::LogEvent() {}

LogEvent::LogEvent(const char* file, int32_t line, int32_t elapse,
                   uint32_t threadId, uint32_t fiberId, uint64_t time)
    : m_file(file)
    , m_line(line)
    , m_elapse(elapse)
    , m_threadId(threadId)
    , m_fiberId(fiberId)
    , m_time(time) {
}

// ==================== LogAppender ====================

LogAppender::~LogAppender() {}

// ==================== Logger ====================

Logger::Logger(const std::string& name)
    : m_name(name)
    , m_level(LogLevel::DEBUG) {
    m_formatter.reset(new LogFormatter("%d [%p] %f %l %m %n"));
}

void Logger::addAppender(LogAppender::ptr appender) {
    if (!appender) {
        return;
    }
    if (!appender->getFormatter()) {
        appender->setFormatter(m_formatter);
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
    if (!appender) {
        return;
    }
    for (std::list<LogAppender::ptr>::iterator it = m_appenders.begin(); it != m_appenders.end(); ++it) {
        if (*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    if (!event || level < m_level) {
        return;
    }
    std::shared_ptr<Logger> self = shared_from_this();
    for (std::list<LogAppender::ptr>::iterator it = m_appenders.begin(); it != m_appenders.end(); ++it) {
        (*it)->log(self, level, event);
    }
}

void Logger::debug(LogEvent::ptr event) { log(LogLevel::DEBUG, event); }
void Logger::info(LogEvent::ptr event)  { log(LogLevel::INFO, event); }
void Logger::warn(LogEvent::ptr event)  { log(LogLevel::WARN, event); }
void Logger::error(LogEvent::ptr event) { log(LogLevel::ERROR, event); }
void Logger::fatal(LogEvent::ptr event) { log(LogLevel::FATAL, event); }

// ==================== Appenders ====================

FileLogAppender::FileLogAppender(const std::string& filename)
    : m_filename(filename) {
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    if (level >= m_level && m_formatter) {
        m_filestream << m_formatter->format(logger, level, event);
    }
}

bool FileLogAppender::reopen() {
    if (m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename.c_str());
    return !m_filestream;
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    if (level >= m_level && m_formatter) {
        std::cout << m_formatter->format(logger, level, event);
    }
}

// ==================== LogFormatter ====================

LogFormatter::LogFormatter(const std::string& pattern)
    : m_pattern(pattern) {
    init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    std::stringstream ss;
    for (std::vector<FormatItem::ptr>::const_iterator it = m_items.begin(); it != m_items.end(); ++it) {
        (*it)->format(ss, logger, level, event);
    }
    return ss.str();
}

void LogFormatter::init() {
    // tokens: (key_or_literal, fmt, type) 
    // type = 0: 普通字符串片段 
    // type = 1: 格式占位符（如 m/p/d）
    std::vector<std::tuple<std::string, std::string, int>> tokens;
    // 累积普通文本（非 '%' 开头的部分）
    std::string literal;

    // 第一步：把 pattern 扫描并拆成 tokens
    for (size_t i = 0; i < m_pattern.size(); ++i) {
        // 普通字符，先累计到 literal
        if (m_pattern[i] != '%') {
            literal.push_back(m_pattern[i]);
            continue;
        }

        // "%%" -> literal '%'
        if (i + 1 < m_pattern.size() && m_pattern[i + 1] == '%') {
            literal.push_back('%');
            ++i;
            continue;
        }
        // 例如解析 %d{%Y-%m-%d}，则literal为%d  TODO 
        if (!literal.empty()) {
            // 先把之前累计的普通文本落入 tokens
            tokens.push_back(std::make_tuple(literal, std::string(), 0));
            literal.clear();
        }

        // 解析一个占位符，支持 %x 和 %x{fmt}
        size_t n = i + 1;
        int fmt_status = 0; // 0: 普通占位符，1: 带格式参数的占位符，2: 带格式参数的占位符结束 
        size_t fmt_begin = 0; // 带格式参数的占位符开始位置  如果%d{%Y-%m-%d}，则fmt_begin为{ 的位置  
        std::string key; // 如果是%d，则key为d  
        std::string fmt; // 如果是%d{%Y-%m-%d}，则fmt为{%Y-%m-%d} 如果%d，则fmt为空   

        while (n < m_pattern.size()) {
            if (fmt_status == 0) {
                if (m_pattern[n] == '{') {
                    // 进入带格式参数的占位符解析，例如 %d{%Y-%m-%d}
                    key = m_pattern.substr(i + 1, n - i - 1);
                    fmt_status = 1;
                    fmt_begin = n;
                    ++n;
                    continue;
                }
                if (!std::isalpha(static_cast<unsigned char>(m_pattern[n]))) {
                    // %m%d  当m_pattern[n]为%时，则结束当前占位符   转到fmt_status = 0  
                    break;
                }
            } else if (fmt_status == 1) {
                if (m_pattern[n] == '}') {
                    // 提取花括号内部格式串
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    fmt_status = 2;
                    break;
                }
            }
            ++n;
        }

        if (fmt_status == 0) {
            // 普通占位符：%m / %p / %t ...
            key = m_pattern.substr(i + 1, n - i - 1);
            tokens.push_back(std::make_tuple(key, std::string(), 1));
            i = n - 1;
        } else if (fmt_status == 1) {
            // %x{... 未闭合，记录一个错误占位，避免崩溃
            tokens.push_back(std::make_tuple("<<pattern_error>>", std::string(), 0));
            i = n;
        } else {
            // 带 fmt 的占位符：%d{...}
            tokens.push_back(std::make_tuple(key, fmt, 1));
            i = n;
        }
    }

    // 把结尾剩余普通文本补入 tokens
    if (!literal.empty()) {
        tokens.push_back(std::make_tuple(literal, std::string(), 0));
    }

    // creators: 占位符 key -> FormatItem 构造器
    static std::map<std::string, FormatItemCreator> creators;
    if (creators.empty()) {
        creators["m"] = [](const std::string& fmt) { return FormatItemPtr(new MessageFormatItem(fmt)); };
        creators["p"] = [](const std::string& fmt) { return FormatItemPtr(new LevelFormatItem(fmt)); };
        creators["r"] = [](const std::string& fmt) { return FormatItemPtr(new ElapseFormatItem(fmt)); };
        creators["c"] = [](const std::string& fmt) { return FormatItemPtr(new NameFormatItem(fmt)); };
        creators["t"] = [](const std::string& fmt) { return FormatItemPtr(new ThreadIdFormatItem(fmt)); };
        creators["n"] = [](const std::string& fmt) { return FormatItemPtr(new NewLineFormatItem(fmt)); };
        creators["f"] = [](const std::string& fmt) { return FormatItemPtr(new FileNameFormatItem(fmt)); };
        creators["l"] = [](const std::string& fmt) { return FormatItemPtr(new LineFormatItem(fmt)); };
        creators["d"] = [](const std::string& fmt) { return FormatItemPtr(new DateTimeFormatItem(fmt)); };
        creators["N"] = [](const std::string& fmt) { return FormatItemPtr(new FiberIdFormatItem(fmt)); };
    }

    // 第二步：把 tokens “编译”成可执行的 m_items
    m_items.clear();
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (std::get<2>(tokens[i]) == 0) {
            // 普通文本，直接创建 StringFormatItem
            m_items.push_back(FormatItemPtr(new StringFormatItem(std::get<0>(tokens[i]))));
            continue;
        }

        const std::string& key = std::get<0>(tokens[i]);
        std::map<std::string, FormatItemCreator>::const_iterator it = creators.find(key);
        if (it == creators.end()) {
            // 未识别占位符，保留可见错误文本，便于排查 pattern
            m_items.push_back(FormatItemPtr(new StringFormatItem("<<error_format_item:%" + key + ">>")));
        } else {
            // 创建对应的格式项对象（fmt 仅对部分占位符生效，例如 %d{...}）
            m_items.push_back(it->second(std::get<1>(tokens[i])));
        }
    }
}

} // namespace sylar