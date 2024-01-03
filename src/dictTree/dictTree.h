#ifndef __DICTTREE_H__
#define __DICTTREE_H__

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <regex>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <memory>
#include <functional>

using std::string;

namespace wd 
{
    // 判断一个 string 是否是中文字符
    bool containsNoChinese(const string& item);

    // 读取文件 invertIndex.lib 将其分为若干个子文件
    void splitFile(string _invertIndex_dir, string save_dir);

    // Trie 节点
    class TrieNode 
    {   
        public:
            TrieNode() {
                ifValueNode = false;
            }

            explicit TrieNode(bool _ifLeafNode, char _curChar) : ifValueNode(_ifLeafNode), curChar(_curChar) {}

        public:
            bool ifValueNode;                                                           // 当前节点是否代表了一个词的路径结束
            char curChar;                                                               // 当前节点对应的字符
            std::unordered_map< char, std::shared_ptr<TrieNode> > childrens;            // 当前节点的childrens映射关系
    };

    // 字典树，向外提供增加和搜索的接口
    class DictTree
    {
        public:
            DictTree() { root = std::make_shared<TrieNode>(); }

            // 添加 string
            void insert(const string& word);

            // 搜索前缀 string
            std::vector<string> search(const string& word);

            // 计算 DictTree 的 valueNode 数量
            int count();

        private:
            std::shared_ptr<TrieNode> root;
            std::mutex mut;
    };

    // Mapper 类
    class Mapper
    {
        public:
            explicit Mapper(std::shared_ptr<DictTree> _dictTree) : dictTree(_dictTree) {};

            void operator() (const std::string filePath);

        private:
            std::shared_ptr<DictTree> dictTree;
    };

    // Reducer 类
    class Reducer 
    {
        public:
            explicit Reducer(std::shared_ptr<DictTree> _dictTree) : dictTree(_dictTree) {};

            void operator() ();

        private:
            std::shared_ptr<DictTree> dictTree;
    };

    // 通过 Map - Reducer 模式构建字典树
    std::shared_ptr<DictTree> mapReduce(string word_file_dir);
}

#endif