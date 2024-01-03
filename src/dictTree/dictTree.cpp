#include "dictTree.h"

namespace wd
{
    bool containsNoChinese(const string& item) {

        std::regex pattern("[a-zA-Z0-9\\p{P}]");
        return std::regex_search(item, pattern);
    }

    void splitFile(string _invertIndex_dir, string save_dir) {

        std::ifstream inputFile(_invertIndex_dir);
        std::string line;
        std::vector<std::string> items;

        int itemCount = 0;
        int fileCount = 0;

        const int MAX_ITEMS_PER_FILE = 10000;

        // 创建保存文件的文件夹
        std::filesystem::create_directory(save_dir);

        while (std::getline(inputFile, line)) {

            std::istringstream iss(line);
            std::string firstItem;

            if (iss >> firstItem) {

                if (containsNoChinese(firstItem)) {
                    continue;
                }
                
                items.push_back(firstItem);
                itemCount++;

                // 达到每个文件保存的最大数据项数量时，保存到一个文件中
                if (itemCount == MAX_ITEMS_PER_FILE) {

                    std::string filePath = save_dir + "/index" + std::to_string(fileCount) + ".txt";
                    std::ofstream outputFile(filePath, std::ios::binary | std::ios::app);

                    for (const auto& item : items) {
                        outputFile << item << std::endl;
                    }

                    outputFile.close();

                    items.clear();
                    itemCount = 0;
                    fileCount++;
                }
            }
        }

        inputFile.close();

        // 如果还有剩余数据，保存到最后一个文件
        if (!items.empty()) {
            std::string filePath = save_dir + "/index" + std::to_string(fileCount) + ".txt";
            std::ofstream outputFile(filePath, std::ios::binary | std::ios::app);

            for (const auto& item : items) {
                outputFile << item << std::endl;
            }

            outputFile.close();   
        }
    }


    void DictTree::insert(const string& word) {

        // 线程安全加锁
        std::lock_guard<std::mutex> lock(mut);

        std::shared_ptr<TrieNode> current = root;

        for (const auto& ch : word) {

            if (current -> childrens.find(ch) == current -> childrens.end()) {

                current -> childrens[ch] = std::make_shared<TrieNode>(false, ch);
            }

            current = current -> childrens[ch];
        }

        // 最后一个节点是代表了值的节点
        current -> ifValueNode = true;
    }

    std::vector<string> DictTree::search(const string& word) {

        std::shared_ptr<TrieNode> current = root;

        for (const auto& ch : word) {

            if (current -> childrens.find(ch) == current -> childrens.end()) {
                return std::vector<string>();
            }

            current = current -> childrens[ch];
        }

        // 返回的数据
        std::vector<string> results;

        // 深度遍历所有代表了值的节点
        std::function<void (std::shared_ptr<TrieNode>, string) > searchHelper 
            = [&] (std::shared_ptr<TrieNode> curNode, string preWord) -> void {

            if (curNode -> ifValueNode) {
                results.emplace_back(preWord + curNode -> curChar);
            }

            for (auto it = curNode -> childrens.begin(); it != curNode -> childrens.end(); it++) {

                searchHelper(it -> second, preWord + it -> first);
            }
        };

        searchHelper(current, word);

        return results;
    }

    int DictTree::count() {

        // 线程安全加锁
        std::lock_guard<std::mutex> lock(mut);

        std::shared_ptr<TrieNode> current = root;

        int ret = 0;

        std::function< void (std::shared_ptr<TrieNode>) > countHelper 
            = [&] (std::shared_ptr<TrieNode> curNode) -> void {

                if (curNode -> ifValueNode) {
                    ret++;
                }

                for (auto it = curNode -> childrens.begin(); it != curNode -> childrens.end(); it++) {

                    countHelper(it -> second);
                }
        };

        countHelper(current);

        return ret;
    }

    void Mapper::operator() (const std::string filePath) {

        std::ifstream file(filePath);
        std::string line;

        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            std::string firstItem;

            if (iss >> firstItem) {

                if (firstItem.size() < 2) {
                    continue;
                }

                dictTree -> insert(firstItem);
            }
        }

        file.close();
    }

    void Reducer::operator() () {

        const auto wordNum = dictTree -> count();

        std::cout << "the word num is: " << wordNum << std::endl;
    }

    std::shared_ptr<DictTree> mapReduce(string word_file_dir) {

        std::vector<string> filenames;
        for (const auto& entry : std::filesystem::directory_iterator(word_file_dir)) {

            if (entry.is_regular_file()) {

                std::cout << "Read index file: " << entry.path().string() << std::endl;
                filenames.push_back(entry.path().string());
            }
        }

        std::shared_ptr<DictTree> dictTree = std::make_shared<DictTree>();

        std::vector<std::thread> threads;
        Mapper mapper(dictTree);
        std::mutex reduceMut;
        Reducer reducer(dictTree);

        // 每个 index 文件新建一个线程进行读取
        for (const auto& filename : filenames) {
            threads.emplace_back([&mapper, &reducer, &reduceMut, filename]() {

                mapper(filename);
                std::lock_guard<std::mutex> lock(reduceMut);
                reducer();
            });
        }

        // 等待所有线程执行完成
        for (auto& thread : threads) {
            thread.join();
        }

        return dictTree;
    }

}