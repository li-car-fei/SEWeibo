#include "Event.h"
#include <sstream>
#include <regex>
#include <queue>
#include <vector>
#include <algorithm>
#include <iterator>
using std::back_inserter;
using std::priority_queue;
using std::regex;
using std::regex_replace;
using std::stringstream;
using std::vector;

namespace wd
{
    Event::Event(const string &doc, SplitTool &splitTool, set<string> &stopWords, unordered_map<string, int>& emotionWords)
        : _doc(doc), _splitTool(splitTool), _stopWords(stopWords), _emotionWords(emotionWords)
    {
        processDoc();
    }

    // 使用cppjieba，对格式化文档进行处理，统计单词频率
    void Event::processDoc()
    {
        stringstream ss(_doc);
        string word;
        while (ss >> word)
        {
            if ("<content>" == word)
            {
                ss >> _content;
                //统计词频，累计计算情感值
                storeWordsMap(_content);
            }
            else if ("<like>" == word)
            {
                ss >> _like;
            }
            else if ("<fans>" == word)
            {
                ss >> _fans;
            }
            else if ("<focus>" == word)
            {
                ss >> _focus;
            }
            else if ("<province>" == word)
            {
                ss >> _province;
                //统计词频
                storeWordsMap(_province);
            }
        }

        //如果没有上述标签，则直接将整个event存入，即查询时调用这部分
        if (_wordsMap.empty())
        {
            storeWordsMap(_doc);
        }

        // 获取单词总数
        for (auto &e : _wordsMap)
        {
            _totalWords += e.second;
        }

        // 计算平均情感值
        _emotionAvg = double(_emotionScores) / _totalWords;

        // 计算出前K频率的词
        calcTopK(TOPK_NUMBER);

    }

    // 根据查询语句中的词，生成当前 event 的摘要
    void Event::makeSummary(const map<string,int>& wordsMap)
    {
        _summary.clear();
        for(auto& e: wordsMap)
        {
            size_t pos = _doc.find(e.first);
            if (pos != _doc.npos)
            {
                _summary += _doc.substr(pos, 100); 
            }
        }
    }


    // 输入一个string，调用 cppjieba 进行分词，并统计频率
    void Event::storeWordsMap(const string &s)
    {
        //注意此处不能采用正则表达式，因为一些中英文的停止词也不能够算进去
        vector<string> vec = _splitTool.cut(s);

        for (auto &e : vec)
        {   
            // stopword 不纳入计数
            if (_stopWords.find(e) != _stopWords.end())
                continue;
            // 词频计算
            _wordsMap[e]++;

            // 情感值累加
            if(_emotionWords.find(e) != _emotionWords.end()) {
                _emotionScores += _emotionWords[e];
            }
        }

    }

    struct Comp
    {
        bool operator()(const pair<string, int> &lhs, const pair<string, int> &rhs)
        {
            return lhs.second > rhs.second; // 降序，形成小顶堆
        }
    };

     // 计算出前K频率的词
    void Event::calcTopK(size_t k)
    {
        /**
         * priority_queue: 优先队列，底层利用 heap 算法进行排序
         * priority_queue<Type, Container, Functional> Type: 存储的元素，Container: 底层容器类型（能用heap算法），Functional: 增删元素时用于排序的算法
        */
        priority_queue<pair<string, int>, vector<pair<string, int>>, Comp> priQue;
        for (auto &e : _wordsMap)
        {
            // heap 算法自动排序
            priQue.push(e);

            // 只要最高频率的 k 个单词
            if (priQue.size() > k)
            {
                priQue.pop();
            }
        }

        // 赋值给_topWords
        while (!priQue.empty())
        {
            _topWords.push_back(priQue.top().first);
            priQue.pop();
        }

        std::sort(_topWords.begin(), _topWords.end());

    }

    string Event::getDoc() const
    {
        return _doc;
    }

    // 返回应用，避免内存损耗
    map<string, int> &Event::getWordsMap()
    {
        return _wordsMap;
    }

    bool operator==(const Event &lhs, const Event &rhs)
    {
        vector<string> vec1 = lhs._topWords;
        vector<string> vec2 = rhs._topWords;
        vector<string> intersection;
        std::set_intersection(vec1.begin(), vec1.end(), vec2.begin(), vec2.end(), back_inserter(intersection));

        // 当前K个高频词都相同，认为两个 event 相同
        if (intersection.size() == TOPK_NUMBER)
            return true;
        else
            return false;
    }

} // namespace wd