#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <map>
#include <iomanip>
#include <string>
using namespace std;

// 定义哈夫曼树节点结构
struct HuffmanNode {
    unsigned char byte;     // 字节值
    int frequency;         // 词频
    HuffmanNode* left;     // 左子节点
    HuffmanNode* right;    // 右子节点
    
    HuffmanNode(unsigned char b, int freq) : 
        byte(b), frequency(freq), left(nullptr), right(nullptr) {}
};

class HuffmanCompression {
private:
    vector<HuffmanNode*> heap;
    
    // 堆的比较函数：词频小的优先，词频相同时按字节值排序
    static bool compareNodes(HuffmanNode* a, HuffmanNode* b) {
        if (a->frequency == b->frequency) {
            return a->byte < b->byte;  // 词频相同时，字节值小的优先
        }
        return a->frequency < b->frequency;  // 词频小的优先
    }
    
    // 向下调整堆
    void heapify(int index, int size) {
        int smallest = index;
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        
        // 找到最小值的索引
        if (left < size && compareNodes(heap[left], heap[smallest])) {
            smallest = left;
        }
        if (right < size && compareNodes(heap[right], heap[smallest])) {
            smallest = right;
        }
        
        // 如果最小值不是当前节点，交换并继续向下调整
        if (smallest != index) {
            swap(heap[index], heap[smallest]);
            heapify(smallest, size);
        }
    }
    
    // 建立最小堆
    void buildMinHeap() {
        for (int i = heap.size() / 2 - 1; i >= 0; i--) {
            heapify(i, heap.size());
        }
    }
    
    // 获取并删除堆顶元素
    HuffmanNode* extractMin() {
        if (heap.empty()) return nullptr;
        
        HuffmanNode* minNode = heap[0];
        heap[0] = heap.back();
        heap.pop_back();
        
        if (!heap.empty()) {
            heapify(0, heap.size());
        }
        
        return minNode;
    }

public:
    // 添加新的文件读取和词频统计方法
    vector<pair<unsigned char, int>> getFrequenciesFromFile(const string& filename) {
        ifstream file(filename, ios::binary);
        if (!file) {
            wcerr << L"无法打开文件: " << filename.c_str() << endl;
            return {};
        }

        // 统计字节频率
        map<unsigned char, int> freqMap;
        char byte;
        while (file.get(byte)) {
            freqMap[static_cast<unsigned char>(byte)]++;
        }
        file.close();

        // 转换为vector
        vector<pair<unsigned char, int>> frequencies;
        for (const auto& pair : freqMap) {
            frequencies.emplace_back(pair.first, pair.second);
        }

        return frequencies;
    }


    // 构建哈夫曼树
    HuffmanNode* buildHuffmanTree(const vector<pair<unsigned char, int>>& frequencies) {
        // 初始化堆
        for (const auto& pair : frequencies) {
            heap.push_back(new HuffmanNode(pair.first, pair.second));
        }
        
        // 建立最小堆
        buildMinHeap();
        
        // 构建哈夫曼树
        while (heap.size() > 1) {
            HuffmanNode* left = extractMin();
            HuffmanNode* right = extractMin();
            
            // 创建新的父节点，频率为子节点之和
            // 父节点的字节值取两个子节点中的最大值
            unsigned char maxByte = max(left->byte, right->byte);
            HuffmanNode* parent = new HuffmanNode(maxByte, 
                                                left->frequency + right->frequency);
            
            // 设置子节点（频率小的在左边）
            parent->left = left;
            parent->right = right;
            
            // 将新节点添加到堆中
            heap.push_back(parent);
            heapify(0, heap.size());
        }
        
        return heap.empty() ? nullptr : heap[0];
    }
    

    // 计算WPL（加权路径长度）
    int calculateWPL(HuffmanNode* root, int depth = 0) {
        if (!root) return 0;
        if (!root->left && !root->right) {
            return root->frequency * depth;
        }
        return calculateWPL(root->left, depth + 1) + 
               calculateWPL(root->right, depth + 1);
    }
    

    // 打印排序后的词频统计
    void printFrequencyStats(const vector<pair<unsigned char, int>>& frequencies) {
        cout << "词频统计列表(按频率排序)：" << endl;
        vector<pair<unsigned char, int>> sortedFreq = frequencies;
        sort(sortedFreq.begin(), sortedFreq.end(), 
            [](const pair<unsigned char, int>& a, const pair<unsigned char, int>& b) {
                if (a.second == b.second) return a.first < b.first;
                return a.second < b.second;
            });
            
        for (const auto& pair : sortedFreq) {
            cout << "字节值: " << (int)pair.first 
                 <<" 频率: " << pair.second << endl;
        }
    }

     // 添加获取文件大小的方法
     long getFileSize(const string& filename) {
        ifstream file(filename, ios::binary | ios::ate);
        if (!file) return 0;
        return file.tellg();
    }

    // 添加计算压缩率的方法
    double calculateCompressionRatio(const string& filename, int wpl) {
        long originalSize = getFileSize(filename) * 8; // 原始文件大小（位）
        if (originalSize == 0) return 0.0;
        return (1.0 - static_cast<double>(wpl) / originalSize) * 100;
    }

};




// 使用示例
int main() {
    HuffmanCompression huffman;
    string filename;
    
    cout << "请输入要压缩的文件路径: ";
    std::getline(cin, filename);

    // 获取词频统计
    vector<pair<unsigned char, int>> frequencies = huffman.getFrequenciesFromFile(filename);
    
    if (frequencies.empty()) {
        cout << "文件为空或无法读取！" << endl;
        return 1;
    }

    // 打印文件基本信息
    cout << "\n文件大小: " << huffman.getFileSize(filename) << " 字节" << endl;
    cout << "不同字符数量: " << frequencies.size() << endl << endl;
    
    // 打印排序后的词频统计
    huffman.printFrequencyStats(frequencies);
    
    // 构建哈夫曼树
    HuffmanNode* root = huffman.buildHuffmanTree(frequencies);
    
    // 计算并打印WPL和压缩率
    int wpl = huffman.calculateWPL(root);
    cout << "\n哈夫曼树的WPL值：" << wpl << endl;
    
    double compressionRatio = huffman.calculateCompressionRatio(filename, wpl);
    cout << "预计压缩率：" << fixed << setprecision(2) << compressionRatio << "%" << endl;
    
    return 0;
}