#include "Configuration.h"
#include "WordQuery.h"
#include "./redis/RedisClient.h"
#include "./log/Log.h"
#include "./net/TcpServer.h"
#include "./threadpool/Threadpool.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <set>
#include <functional>
using std::bind;
using std::cout;
using std::endl;
using std::ifstream;
using std::set;
using std::stringstream;
using namespace wd;

// 单例模式
Configuration *Configuration::_pInstance = new Configuration("/home2/ljh/LightSE/online/conf/setting.conf");

// 单例模式
Log* Log::_plog = new Log(Configuration::getpInstance()->getConfigMap()["Logfile"]);

// Redis 缓存
RedisClient redisClient(
    Configuration::getpInstance()->getConfigMap()["IP"], 
    stoi(Configuration::getpInstance()->getConfigMap()["redis_port"]), 
    Configuration::getpInstance()->getConfigMap()["redis_password"]);

// 查询所用，内部用到缓存
WordQuery wordQuery(
    Configuration::getpInstance()->getConfigMap()["stop_words.utf8"],
    Configuration::getpInstance()->getConfigMap()["emotion_words"],
    Configuration::getpInstance()->getConfigMap()["NewEvents_dir"], 
    Configuration::getpInstance()->getConfigMap()["Newoffset_dir"], 
    Configuration::getpInstance()->getConfigMap()["invertIndex_dir"],
    Configuration::getpInstance()->getConfigMap()["emotionScore_dir"]);

// 定义全局变量：线程池
Threadpool *pthreadpool = NULL;

// tcp 连接成功的回调
void ConnectionCallBack(const TcpConnectionPtr &ptr)
{
    logInfo(">>client has connected " + ptr->Address());
}

// 接收到 tcp 连接写入到文件（传来内容）的回调：处理传递来的内容
void MessageCallBack(const TcpConnectionPtr &ptr)
{   
    // ptr->recv() 读取tcp传过来的内容，并赋值给 msg （string）
	string msg(ptr->recv());
    // 找到换行符，只取换行符前的内容
	size_t pos = msg.find('\n');
	msg = msg.substr(0, pos);
    logInfo("search event: " + msg);
    if (msg.size() <= 1) return ptr->sendInEventLoop("");

    // 查询网页的任务交给子线程，传递一个 绑定了WordQuery实例对象并指定了参数的函数，在线程池中执行；将redis客户端传递进去
    pthreadpool->addTask(std::bind(&WordQuery::doQuery, &wordQuery, redisClient, msg, ptr));
}

// tcp 连接关闭的回调
void CloseCallBack(const TcpConnectionPtr &ptr)
{
    logInfo(">>client has broken up " + ptr->Address());
}

int main()
{
    //启动线程池，用于处理任务，在业务函数中向线程池添加任务，与 Server 剥离；
    Threadpool threadpool(
        stoi(Configuration::getpInstance()->getConfigMap()["threadNum"]), 
        stoi(Configuration::getpInstance()->getConfigMap()["TaskQueSize"]));
    pthreadpool = &threadpool;
    pthreadpool->start();

    //启动服务器
    TcpServer server(
        Configuration::getpInstance()->getConfigMap()["IP"], 
        stoi(Configuration::getpInstance()->getConfigMap()["port"]));

    // TCP连接成功、客户端发送信息的处理函数、TCP关闭连接 三者的处理函数设置
    server.setConnectionCallBack(ConnectionCallBack);
    server.setMessageCallBack(MessageCallBack);
    server.setCloseCallBack(CloseCallBack);
    server.start();
    return 0;
}