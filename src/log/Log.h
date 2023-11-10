#ifndef __LOG_H_
#define __LOG_H_

#include<iostream>
#include<string>
#include<log4cpp/Category.hh>
#include<log4cpp/Appender.hh>
#include<log4cpp/OstreamAppender.hh>
#include<log4cpp/FileAppender.hh>
#include<log4cpp/RollingFileAppender.hh>
#include<log4cpp/PatternLayout.hh>
#include<log4cpp/Priority.hh>
#include<string>
#include<sstream>
using namespace std;
using std::string;

namespace wd 
{
    // log 信息类型
    enum LogPriority {EMERG, FATAL, ALERT, CRIT, ERROR, WARN, NOTICE, INFO, DEBUG };

    class Log
    {
        public:
            // 单例模式，内部保存一个指针指向本类；定义获取接口为静态函数；
		    static Log* getInstance();   
		    static void destroy();

		    void setPriority(LogPriority priority);
		    void fatal(const string& msg);
		    void error(const string& msg);
		    void warn(const string& msg);
		    void info(const string& msg);
		    void debug(const string& msg);
	private:
		    static Log* _plog;
		    log4cpp::Category& rootCategory;
	private:
		    Log(const string& filepath);
		    ~Log();
    };

    // 定义一些宏定义，快速进行 log 写入
    #define logSetpriority(priority) Log::getInstance()->setPriority(priority)
    #define logError(msg) Log::getInstance()->error(msg)
    #define logWarn(msg) Log::getInstance()->warn(msg)
    #define logInfo(msg) Log::getInstance()->info(msg)
    #define logDebug(msg) Log::getInstance()->debug(msg)
    #define logFatal(msg) Log::getInstance()->fatal(msg)
    #define logDestroy() Log::destroy()

}

#endif // __LOG_H_