#ifndef __RedisClient_H__
#define __RedisClient_H__

#include <sw/redis++/redis++.h>
#include <iostream>
#include <memory>
#include <string>
#include "Log.h"

namespace wd {

    using namespace sw::redis;

    /**
    * Redis 操作相关
    */
    class RedisClient {

    public:
        RedisClient(const std::string& ip_, std::size_t port_, const std::string& password_ = "", const size_t pool_size_ = 3);

        void setOptions();

        void connect();

        // 获取缓存
        std::pair<bool, std::string> get(const std::string& s);

        // 设置缓存
        void set(const std::string& searchStr, const std::string& result);

    private:
        std::string ip;
        std::size_t port;
        std::string password;
        size_t pool_size;
        ConnectionOptions connection_options;
        ConnectionPoolOptions pool_options;
        std::shared_ptr<Redis> redis_;
        bool connected = false;
    };

}


#endif // __RedisClient_H__