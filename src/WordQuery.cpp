#include "WordQuery.h"
#include <cmath>
#include <algorithm>
#include <iterator>
#include <json/json.h>
#include <string>
using std::make_pair;
using std::ofstream;

namespace wd
{   
    // 构造函数
    WordQuery::WordQuery(const string &stopWords_dir, const string& emotionWords_dir, const string &ripePage_dir, const string &offset_dir, const string &invertIndex_dir, const string& emotionScores_dir)
        : _stopWords_dir(stopWords_dir), _emotionSwords_dir(emotionWords_dir), _ripePage_dir(ripePage_dir), _offset_dir(offset_dir), _invertIndex_dir(invertIndex_dir), _emotionScores_dir(emotionScores_dir)
    {
        readInfoFromFile();
    }

    // 根据文件地址，读取 stopwords、网页库、偏移库、倒排索引库
    void WordQuery::readInfoFromFile()
    {
        // 读取停词库
        ifstream stop_words_File(_stopWords_dir);
        ifstream emotion_words_file(_emotionSwords_dir);
        if (!stop_words_File.good() || !emotion_words_file.good())
        {
            logError(string(__FILE__).append(":").append(std::to_string(__LINE__).append(" file open ERROR!")));
            exit(-1);
        }
        string words;
        while (getline(stop_words_File, words))
        {
            _stopWords.insert(words);
        }
        _stopWords.insert(" ");

        // 读取情感词典
        string line;
        while (getline(emotion_words_file, line)) {
            std::istringstream iss(line);
            string word;
            int score;

            if(iss >> word >> score) {
                _emotionWords.insert({word, score});
            }
        }

        // 读取网页库，偏移库，倒排索引库，情感得分表
        ifstream pageFile(_ripePage_dir);
        ifstream offsetFile(_offset_dir);
        ifstream invertIndexFile(_invertIndex_dir);
        ifstream emotionScoreFile(_emotionScores_dir);
        if (!pageFile.good() || !offsetFile.good() || !invertIndexFile.good() || !emotionScoreFile.good())
        {
            logError(string(__FILE__).append(":").append(std::to_string(__LINE__).append(" file open ERROR!")));
            exit(-1);
        }

        // 读取网页库与偏移库
        {
            string s;
            int docid;
            off_t beg, end;
            while (getline(offsetFile, s))
            {   
                // 从偏移记录文件中获取每个 <event>...<event> 的存储区间
                stringstream ss(s);
                ss >> docid >> beg >> end;
                //根据偏移库读取<event> ... </event>
                char buf[65536] = {0};
                pageFile.read(buf, end - beg);
                string doc(buf);

                // 解析 <event>...<event>，这里不用再算一遍情感得分了；只再解析一遍文本，获得词频；
                Event event(doc, _splitTool, _stopWords, _emotionWords);

                // 存储偏移记录
                _offsetLib[docid] = make_pair(beg, end);

                // std::move() 高效移动，由于Event中用的都是STL容器，会自动合成移动构造函数；
                _events.push_back(std::move(event));
            }
        }

        // 读取倒排索引库
        {
            string s;
            string word;
            int docid;
            double w;
            while (getline(invertIndexFile, s))
            {
                stringstream ss(s);
                ss >> word;
                while (ss)
                {
                    ss >> docid >> w;
                    _invertIndexTable[word][docid] = w;
                }
            }
        }

        // 读取情感得分表
        {
            string s;
            int docid;
            double score;
            while (getline(emotionScoreFile, s))
            {
                stringstream ss(s);
                ss >> docid >> score;
                emotionScores.push_back(score);
            }
        }

        stop_words_File.close();
        emotion_words_file.close();
        offsetFile.close();
        pageFile.close();
        invertIndexFile.close();
        emotionScoreFile.close();
        logInfo("停词库，情感词表，网页库，偏移库，倒排索引库，情感得分表 读取数据成功!");
    }

    // 执行查询
    void WordQuery::doQuery(RedisClient& redisClient, const string &s, const TcpConnectionPtr &ptr)
    {
        // 先查询缓存
        auto redisResult = redisClient.get(s);
        if (redisResult.first)
        {
            ptr->sendInEventLoop(redisResult.second);
            return;
        }

        // 提取关键字和搜索文本
        auto parseSearchTextResult = parseSearch(s);

        /**
         * 构建返回结果所需：
         *  （1）set<int> eventIds：事件id
         *  （2）map<int, map<string, int>> wordsMap : 词频记录（当前搜索语句所构建的event的） 
         *  （3）map<int, vector<pair<string, double>>> vec ：每个词与其权重（当前搜索语句所构建的event的） 
        */

        set<int> eventIds;
        map<int, map<string, int>> wordsMaps;
        map<int, vector<pair<string, double>>> vecs;

        // 循环遍历关键字和搜索文本，构建结果
        searchThrough(eventIds, wordsMaps, vecs, parseSearchTextResult.first, parseSearchTextResult.second);

        // 根据第一个搜索map和搜索文本的vec 计算余弦排序，形成摘要
        string message = doReturn(eventIds, wordsMaps.begin()->second, vecs.begin()->second, ptr);

        // 设置缓存
        redisClient.set(s, message);

    }

    // 提取关键字和搜索文本
    pair<vector<string>, vector<string>> WordQuery::parseSearch(const string& s)
    {
        std::vector<std::string> keywords;
        std::vector<std::string> searchTexts;

        std::istringstream iss(s);
        std::string word;
        while (iss >> word) {
            if (word == "AND" || word == "OR" || word == "NOT") {
                keywords.push_back(word);
            } else {
                searchTexts.push_back(word);
            }
        }

        // 默认为 "AND"
        keywords.insert(keywords.begin(), "AND");

        return { keywords, searchTexts };
    }

    // 循环遍历关键字和搜索文本，构建结果
    void WordQuery::searchThrough(set<int>& eventIds, map<int, map<string, int>>& wordsMaps, map<int, vector<pair<string, double>>>& vecs, 
                                  const vector<string>& keywords, const vector<string>& searchTexts)
    {
        for (size_t i = 0; i < keywords.size(); ++i)
        {
            const auto& keyword = keywords[i];
            const auto& searchText = searchTexts[i]; 

            // 计算当前 searchText 对应匹配的 inner_eventIds、inner_wordsMap、inner_vec

            // 解析一段搜索文本（将其看做一个event）
            Event event(searchText, _splitTool, _stopWords, _emotionWords);
            auto parseEventResult = parseEvent(event);

            // 根据解析的单词，找到所有匹配的文档ID （包含了查询文本所有单词的文档的Id集合）
            set<int> inner_eventIds = getAllMatchingEvents(parseEventResult.second);
            // 根据情感值筛选（查询语句的情感倾向需与匹配的文档一致）
            getEmotionMatching(inner_eventIds, event.getAvgScore());


            if(keyword == "AND")
            {
                if(eventIds.empty())
                {
                    eventIds = inner_eventIds;
                    for (const auto& i : inner_eventIds)
                    {
                        wordsMaps[i] = parseEventResult.first;
                        vecs[i] = parseEventResult.second;
                    }

                } else 
                {
                    // 求交集
                    set<int> temp;
                    std::set_intersection(eventIds.begin(), eventIds.end(), inner_eventIds.begin(), inner_eventIds.end(), std::inserter(temp, temp.end()));

                    for (const auto& i : eventIds)
                    {
                        if (temp.find(i) == temp.end())
                        {
                            wordsMaps.erase(i);
                            vecs.erase(i);
                        }
                    }

                    eventIds = temp;

                }
            } else if (keyword == "OR")
            {
                // 求并集
                eventIds.insert(inner_eventIds.begin(), inner_eventIds.end());
                for (const auto& i : inner_eventIds)
                {
                    wordsMaps[i] = parseEventResult.first;
                    vecs[i] = parseEventResult.second;
                }

            } else if (keyword == "NOT")
            {
                // 求非集
                for (const auto& i : inner_eventIds) 
                {
                    eventIds.erase(i);
                    wordsMaps.erase(i);
                    vecs.erase(i);
                }
            }

        }
        
    }

    // 解析一段搜索文本（将其看做一个event）
    pair<map<string, int>, vector<pair<string, double>>> WordQuery::parseEvent(Event& event)
    {
        // 用TF-IDF算法计算出查询语句的单词权重向量 vec
        map<string, int> &wordsMap = event.getWordsMap();
        vector<pair<string, double>> vec = getWeightVector(wordsMap);

        return { wordsMap, vec };
    }


    // 根据解析和匹配结果进行返回
    string WordQuery::doReturn(const set<int> &eventIds, const map<string, int> &wordsMap, const vector<pair<string, double>>& vec, const TcpConnectionPtr &ptr)
    {
        string ret;
        if (eventIds.empty())
        {
            // 没有找到对应文章，返回404json
            ret = return404Json();
        }
        else
        {
            // 使用余弦相似度算法对候选文档进行排序
            vector<pair<int, double>> sortedVec;        // <文档id，余弦相似度>
            sortedVec = cosSort(vec, eventIds);

            // 为每一篇文章生成摘要
            makePageSummary(eventIds, wordsMap);
            // json封装数据返回客户端 发送字符串的格式为 json长度\n格式化json字符串
            ret = JsonPackage(sortedVec);
        }


        int sz = ret.size();                            // 返回内容长度
        string message(std::to_string(sz));             // 长度转string
        message.append("\n").append(ret);               // 返回格式：json长度\n格式化json字符串
        ptr->sendInEventLoop(message);                  // tcp连接返回

        return message;
    }

    // 为每一篇查询结果的文章生成摘要
    // pageIds: 查询结果文档Id集合
    // wordsMap: 查询语句的词频记录
    void WordQuery::makePageSummary(const set<int> &eventIds, const map<string, int> &wordsMap)
    {
        for (auto& docid: eventIds)
        {
            _events[docid].makeSummary(wordsMap);
        }
    }

    // 没有找到对应文章，返回404json
    string WordQuery::return404Json()
    {
        Json::Value root;
        Json::Value arr;

        Json::Value elem;
        elem["summary"] = "很抱歉，没有找到相关的微博话题。";
        elem["like"] = "NULL";
        elem["fans"] = "NULL";
        elem["focus"] = "NULL";
        elem["province"] = "NULL";
        arr.append(elem);

        root["files"] = arr;
        Json::StyledWriter writer;
        return writer.write(root);
    }

    // 计算两个向量的余弦相似度
    double WordQuery::countCos(const vector<double> &X, const vector<double> &Y)
    {
        if (X.size() != Y.size())
        {
            logError(string(__FILE__).append(":").append(std::to_string(__LINE__).append(" 权重向量长度不相等!")));
            exit(-1);
        }

        //cos0 = (X*Y) / (|X| * |Y|)
        double up = 0;
        for (size_t i = 0; i < X.size(); i++)
            up += X[i] * Y[i];

        double down = getMold(X) * getMold(Y);
        return up / down;
    }

    //求向量的模长
    double WordQuery::getMold(const vector<double> &vec)
    {
        int n = vec.size();
        double sum = 0.0;
        for (int i = 0; i < n; ++i)
            sum += vec[i] * vec[i];
        return sqrt(sum);
    }

    // json封装数据返回客户端 发送字符串的格式为 json长度\n格式化json字符串
    // sortedVec: <搜索结果文档Id，与查询语句的余弦相似度>
    string WordQuery::JsonPackage(vector<pair<int, double>> &sortedVec)
    {
        Json::Value root;
        Json::Value arr;
        int cnt = 0;
        for (auto &e : sortedVec)
        {
            int docid = e.first;
            Event& event = _events[docid];

            Json::Value elem;
            elem["summary"] = event.getSummary();
            elem["like"] = event.getLike();
            elem["fans"] = event.getFans();
            elem["focus"] = event.getFocus();
            elem["province"] = event.getProvince();
            elem["similarity"] = e.second;
            elem["emotion"] = event.getAvgScore();
            arr.append(elem);
            // 最多返回记录100条
            if (++cnt == 100)
                break;
        }

        root["files"] = arr;
        Json::StyledWriter writer;
        return writer.write(root);
    }

    // 使用余弦相似度算法对候选文档进行排序
    // vec: 查询语句的单词权重向量
    // pageIds: 候选文档ID
    vector<pair<int, double>> WordQuery::cosSort(const vector<pair<string, double>> &vec, const set<int> &eventIds)
    {
        vector<double> weightsVec;                  // 查询语句的单词权重向量
        for (auto &e : vec)
            weightsVec.push_back(e.second);

        // 先计算出每一篇doc对应查询语句的权重向量
        map<int, vector<double>> weightsMap;        // <文章id, 对应查询语句的权重向量>
        for (auto &docid : eventIds)
        {
            for (auto &e : vec)
            {
                string word = e.first;
                double w = _invertIndexTable[word][docid];
                weightsMap[docid].push_back(w);
            }
        }

        // 对文章先后进行排序
        vector<pair<int, double>> sortedVec;        // <文档id，余弦相似度>
        for (auto &e : weightsMap)
        {
            int docid = e.first;
            vector<double> eventVec = e.second;      // 不同event的查询语句的权重向量
            double cos = countCos(eventVec, weightsVec);
            sortedVec.push_back(make_pair(docid, cos));
        }
        // 根据查询语句与event对应单词向量的相似度，进行排序
        std::sort(sortedVec.begin(), sortedVec.end(), [&](const pair<int, double> &lhs, const pair<int, double> &rhs) {
            return lhs.second > rhs.second;
        });

        return sortedVec;
    }

    // 根据查询文本的TF-IDF权重向量，找到符合条件的doc的id
    set<int> WordQuery::getAllMatchingEvents(const vector<pair<string, double>> &vec)
    {
        // 通过倒排索引表查找出 包含 vec 中每个单词的网页id
        vector<set<int>> docid_sets;                    // 存放单词的所在网页id集合
        for (size_t i = 0; i < vec.size(); i++)
        {
            string word = vec[i].first;
            unordered_map<int, double> temp = _invertIndexTable[word];
            set<int> sets;
            for (auto &e : temp)
            {
                sets.insert(e.first);
            }
            docid_sets.push_back(sets);
        }

        // 再将docid_sets中 每个单词所在文档集合 求交集，即包含了查询语句所有单词的 文档的集合
        set<int> ans = docid_sets[0];
        for (size_t i = 1; i < docid_sets.size(); i++)
        {
            set<int> temp;
            std::set_intersection(docid_sets[i].begin(), docid_sets[i].end(), ans.begin(), ans.end(), inserter(temp, temp.begin()));
            swap(ans, temp);
        }

        return ans; // 候选文档id集合
    }

    // 根据查询文本的情感倾向对符合条件的event筛选，只取情感倾向一致的
    void WordQuery::getEmotionMatching(set<int>& eventIds, double searchScore)
    {
        auto it = eventIds.begin();
        while (it != eventIds.end()) 
        {
            // 异或判断两者正负性不同
            if ( ( emotionScores[*it] < 0) ^ (searchScore < 0) )
            {   
                // 删除后返回其下一个元素的迭代器
                it = eventIds.erase(it);
            } else 
            {
                it++;
            }
        }
    }

    // 获取查询文本的权重向量
    vector<pair<string, double>> WordQuery::getWeightVector(const map<string, int> &wordsMap)
    {
        // 先求取map中包含的单词总数
        int sum = 0;
        for (auto &e : wordsMap)
        {
            sum += e.second;
        }

        vector<pair<string, double>> vec;
        for (auto &e : wordsMap)
        {
            string word = e.first;
            double TF = (double)e.second / sum;                     // 该词在输入文本段的权重
            int DF = _invertIndexTable[word].size();                // 有多少篇doc出现该词
            int N = _events.size();                                 // 总共有多少个event
            double IDF = log10((double)N / (DF + 1));
            double w = TF * IDF;
            vec.push_back(make_pair(word, w));                      // 该词的 TF-IDF 权重
        }

        //权重归一化
        double sum2 = 0;
        for (auto &e : vec)
        {
            sum2 += e.second * e.second;
        }
        sum2 = sqrt(sum2);
        for (auto &e : vec)
        {
            e.second /= sum;
        }
        return vec;
    }

} // namespace wd