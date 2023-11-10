#ifndef __WORDQUERY_H__
#define __WORDQUERY_H__
#include "SplitTool.hpp"
#include "Configuration.h"
#include "./net/TcpConnection.h"
#include "./redis/RedisClient.h"
#include "./log/Log.h"
#include "Event.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <set>
#include <unordered_map>
using std::cout;
using std::endl;
using std::ifstream;
using std::set;
using std::string;
using std::stringstream;
using std::unordered_map;

namespace wd
{
    class WordQuery
    {
    public:
        // 构造，从各种库文件读取数据
        WordQuery(const string &stopWords_dir, const string& emotionWords_dir, const string &ripePage_dir, const string &offset_dir, const string &invertIndex_dir, const string& emotionScores_dir);

        // 执行查询，用于绑定线程池的回调函数
        void doQuery(RedisClient&, const string &str, const TcpConnectionPtr&);

    private:
        void readInfoFromFile(); //从停词库，网页库，偏移库，倒排索引库读取数据
        vector<pair<string, double>> getWeightVector(const map<string, int> &);

        // 根据所有词必须匹配，得到搜索备选集
        set<int> getAllMatchingPages(const vector<pair<string, double>> &);

        // 根据情感倾向一致性判断，筛选一遍备选集
        void getEmotionMatching(set<int>& eventIds, double searchScore);

        // 余弦相似度算法
        vector<pair<int, double>> cosSort(const vector<pair<string, double>> &, const set<int>&);
        // 计算余弦相似度
        double countCos(const vector<double> &, const vector<double> &);
        // 求向量模长
        double getMold(const vector<double> &);

        // 将返回数据封装成json格式
        string JsonPackage(vector<pair<int,double>>& );
        // 返回404网页json格式
        string return404Json();

        // 为匹配到的文章生成摘要
        void makePageSummary(const set<int>& pageIds, const map<string,int>& wordsMap);

    private:
        string _stopWords_dir;                                              // 停词库文件地址
        string _emotionSwords_dir;                                          // 情感词典文件地址
        string _ripePage_dir;                                               // event事件库文件地址
        string _offset_dir;                                                 // 存储偏移记录文件地址
        string _invertIndex_dir;                                            // TD-IDF 倒排索引文件地址
        string _emotionScores_dir;                                          // events的情感分记录文件地址

        vector<Event> _events;                                              // 解析一段 <doc>...<doc> 后的结果，解析各部分内容、词频记录等
        unordered_map<int, pair<int, int>> _offsetLib;                       // 偏移库    <文档ID，<网页库文件中存储起始位置, 结束位置>>
        unordered_map<string, unordered_map<int, double>> _invertIndexTable; // 倒排索引库 <单词，<所在文档ID, 单词权重>>
        vector<double> emotionScores;                                        // event情感得分表

        set<string> _stopWords;                                              // 停词库
        unordered_map<string, int> _emotionWords;                            // 情感词典集
        SplitTool _splitTool;                                                // cppjieba分词工具
    };
} // namespace wd

#endif