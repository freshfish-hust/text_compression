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
    map<unsigned char, string> huffmanCodes; // 存储每个字节的哈夫曼编码

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

    // 生成哈夫曼编码
    void generateCodes(HuffmanNode* root, string code = "") {
        if (!root) return;
        
        // 叶子节点
        if (!root->left && !root->right) {
            huffmanCodes[root->byte] = code;
            return;
        }
        
        generateCodes(root->left, code + "0");
        generateCodes(root->right, code + "1");
    }

    // 将二进制字符串转换为字节
    vector<unsigned char> binaryStringToBytes(const string& binaryStr) {
        vector<unsigned char> bytes;
        for (size_t i = 0; i < binaryStr.length(); i += 8) {
            string byte = binaryStr.substr(i, 8);
            // 补足8位
            while (byte.length() < 8) byte += "0";
            bytes.push_back(static_cast<unsigned char>(stoi(byte, nullptr, 2)));
        }
        return bytes;
    }

public:

    void generateCodeTable(const string& filename, HuffmanNode* root) {
        // 生成哈夫曼编码
        huffmanCodes.clear();
        generateCodes(root);

        // 创建编码表文件
        ofstream codeFile("code.txt", ios::binary);
        if (!codeFile) {
            cerr << "无法创建编码表文件！" << endl;
            return;
        }

        // 写入原文件大小
        long fileSize = getFileSize(filename);
        codeFile.write(reinterpret_cast<const char*>(&fileSize), sizeof(fileSize));

        // 写入编码表
        for (const auto& pair : huffmanCodes) {
            unsigned char byte = pair.first;
            string code = pair.second;
            unsigned char codeLength = static_cast<unsigned char>(code.length());
            
            // 将编码转换为字节
            vector<unsigned char> codeBytes = binaryStringToBytes(code);
            
            // 写入字节值
            codeFile.write(reinterpret_cast<const char*>(&byte), 1);
            // 写入编码长度
            codeFile.write(reinterpret_cast<const char*>(&codeLength), 1);
            // 写入编码字节
            codeFile.write(reinterpret_cast<const char*>(codeBytes.data()), 
                          codeBytes.size());
        }
        codeFile.close();
    }

    // 显示编码表内容
    void displayCodeTable() {
        cout << "\n编码表内容（HEX值）：" << endl;
        for (const auto& pair : huffmanCodes) {
            cout << "字节: 0x" << hex << uppercase << setw(2) << setfill('0') 
                 << static_cast<int>(pair.first) << " 编码长度: 0x" 
                 << setw(2) << pair.second.length();
            
            // 转换编码为字节并显示
            vector<unsigned char> codeBytes = binaryStringToBytes(pair.second);
            cout << " 位编码: ";
            for (unsigned char b : codeBytes) {
                cout << "0x" << setw(2) << static_cast<int>(b) << " ";
            }
            cout << dec << endl;
        }
    }

    // 编码文件并显示最后16个字节
    void encodeAndShowLast16Bytes(const string& filename) {
        ifstream inFile(filename, ios::binary);
        if (!inFile) {
            cerr << "无法打开源文件！" << endl;
            return;
        }

        string encodedBits;
        char byte;
        while (inFile.get(byte)) {
            encodedBits += huffmanCodes[static_cast<unsigned char>(byte)];
        }
        inFile.close();

        // 转换为字节
        vector<unsigned char> encodedBytes = binaryStringToBytes(encodedBits);
        
        // 显示最后16个字节
        cout << "\n压缩后文件的最后16个字节：" << hex << uppercase;
        size_t start = encodedBytes.size() > 16 ? encodedBytes.size() - 16 : 0;
        for (size_t i = start; i < encodedBytes.size(); i++) {
            cout << "0x" << setw(2) << setfill('0') 
                 << static_cast<int>(encodedBytes[i]) << " ";
        }
        cout << dec << endl;
    }

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
    
     // 生成编码表文件
    huffman.generateCodeTable(filename, root);
    
     // 显示编码表内容
    //huffman.displayCodeTable();
     
     // 显示压缩后的最后16个字节
    huffman.encodeAndShowLast16Bytes(filename);

    return 0;
}