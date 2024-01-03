#ifndef __EVENT_H__
#define __EVENT_H__
#include "SplitTool.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>
using std::map;
using std::pair;
using std::set;
using std::string;
using std::vector;
using std::unordered_map;

namespace wd
{
    const static size_t TOPK_NUMBER = 10;

    // 存储一个事件的基本信息与统计信息, 也就是 <event>..</event> 的记录与统计信息
    class Event
    {
    public:
        Event(const string &doc, SplitTool &splitTool, set<string>& stopWords, unordered_map<string, int>& emotionWords);

        // 获取整个 <event>...</event>
        string getDoc() const;
        // 获取event内部内容
        string getContent() const { return _content; }
        string getLike() const { return _like; }
        string getFans() const { return _fans; }
        string getFocus() const { return _focus; }
        string getProvince() const { return _province; }

        // 取得词总数
        int getTotalWords() const { return _totalWords; };

        // 取得平均情感值
        double getAvgScore() const { return _emotionAvg; };

        // 获取词频表
        map<string, int> &getWordsMap();

        // 根据查询词生成摘要 
        void makeSummary(const map<string,int>& wordsMap);
        string getSummary() const { return _summary; }

        // 判断两个event是否相等
        friend bool operator==(const Event &lhs, const Event &rhs);


    private:
        // 使用cppjieba，对格式化文档进行处理
        void processDoc();

        // 进行词频统计
        void storeWordsMap(const string &s);

        // topK算法计算频率最高的k个单词
        void calcTopK(size_t k);

    private:
        string _doc;                                    // 整个<event> .. </event>
        string _content;
        string _like;
        string _fans;
        string _focus;
        string _province;

        string _summary;                                // 摘要，返回时自行生成

        int _totalWords;                                // 词总数
        int _emotionScores;                             // 情感累加值
        double _emotionAvg;                             // 情感累加值 /_totalWords 作为该 event 的情感值

        vector<string> _topWords;                       // 该event频率最高的前k个词
        map<string, int> _wordsMap;                     // 该event  <单词，频率>
        SplitTool &_splitTool;                          // cppjieba中文分词库
        set<string> &_stopWords;                        // 停词库
        unordered_map<string, int>& _emotionWords;      // 情感词典集
    };

} // namespace wd

#endif