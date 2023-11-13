#ifndef __LOG_CPP_
#define __LOG_CPP_

#include"Log.h"

/**
 * log4cpp 使用：
 *  （1）rootCategory 是总体的一个设置，所有log实例挂载到上面
 *  （2）Appender 设置log打印到什么地方
 *  （3）Layout 设置日志打印格式
 *  （4）Layout 挂载到 Appender，Appender 再挂载到 Category，形成一个日志实例
*/

namespace wd
{
    Log::Log(const string& filepath)
        : rootCategory(log4cpp::Category::getRoot().getInstance("rootCategory"))
    {

        // 设置打印到命令行的log   
	    log4cpp::OstreamAppender* osAppender = new log4cpp::OstreamAppender("osAppender", &cout);
	    log4cpp::PatternLayout* pLayout1 = new log4cpp::PatternLayout();
	    pLayout1->setConversionPattern("%d: %p %c %x: %m%n");
	    osAppender->setLayout(pLayout1);
	    rootCategory.addAppender(osAppender);
	    rootCategory.setPriority(log4cpp::Priority::DEBUG);

	    // 设置打印到文件的log
	    log4cpp::FileAppender*fileAppender = new log4cpp::FileAppender("fileAppender", filepath);
	    log4cpp::PatternLayout* pLayout2 = new log4cpp::PatternLayout();
	    pLayout2->setConversionPattern("%d: %p %c %x: %m%n");
	    fileAppender->setLayout(pLayout2);
	    rootCategory.addAppender(fileAppender);
	    rootCategory.setPriority(log4cpp::Priority::DEBUG);

        rootCategory.info("Log init success");
    };

    Log::~Log()
    {
        rootCategory.info("Log shutdown ~ ");
        rootCategory.shutdown();
    }

    void Log::destroy()
    {
        if (_plog)
        {
            _plog->rootCategory.info("Log destroy ~ ");
            // 析构 _plog 所指向的对象
            delete _plog;
            // 将 _plog 置空
            _plog = nullptr;
        }
    }

    Log* Log::getInstance()
    {
        return _plog;
    }

    // 重新设置rootCategory的优先级
    void Log::setPriority(LogPriority priority)
    {
	    switch(priority)
	    {
		    case FATAL:rootCategory.setPriority(log4cpp::Priority::FATAL);break;
		    case ERROR:rootCategory.setPriority(log4cpp::Priority::ERROR);break;
		    case WARN:rootCategory.setPriority(log4cpp::Priority::WARN);break;
		    case INFO:rootCategory.setPriority(log4cpp::Priority::INFO);break;
		    case DEBUG:rootCategory.setPriority(log4cpp::Priority::DEBUG);break;
		    default:
				        rootCategory.setPriority(log4cpp::Priority::DEBUG);break;
	    }
    }

    void Log::error(const string& msg)
    {
	    rootCategory.error(msg);
    }

    void Log::warn(const string& msg)
    {
	    rootCategory.warn(msg);
    }

    void Log::info(const string& msg)
    {
	    rootCategory.info(msg);
    }

    void Log::debug(const string& msg)
    {
	    rootCategory.debug(msg);
    }

    void Log::fatal(const string& msg)
    {
	    rootCategory.fatal(msg);
    }

}

#endif // __LOG_CPP_