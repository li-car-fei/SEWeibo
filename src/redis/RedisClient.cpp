#ifndef __RedisClient_CPP__
#define __RedisClient_CPP__

#include "RedisClient.h"

namespace wd {

    RedisClient::RedisClient(const std::string& ip_, std::size_t port_, const std::string& password_, const size_t pool_size_) :
        ip(ip_), port(port_), password(password_), pool_size(pool_size_)
    {
        setOptions();
        connect();
    };

    void RedisClient::setOptions() 
    {
        connection_options.host = ip;
        connection_options.port = port;
        // connection_options.password = password;
        pool_options.size = pool_size;
    }

    void RedisClient::connect() 
    {
        if(!connected) 
        {
            // 连接Redis服务器
            redis_ = std::make_shared<Redis>(connection_options, pool_options);
            connected = true;
        }
    }

    std::pair<bool, std::string> RedisClient::get(const std::string& s) 
    {
        auto val = redis_->get(s);
        if (val) 
        {
            logInfo("redis key exists: " + s);
            return std::make_pair(true, *val);
        }

        logInfo("redis key doesn't exist: " + s);
        return std::make_pair(false, std::string(""));
    }

    void RedisClient::set(const std::string& searchStr, const std::string& result) 
    {
        logInfo("set redis key: " + searchStr);
        redis_->set(searchStr, result);
    }

}


#endif // __RedisClient_CPP__