#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <map>
#include <iomanip>
#include <string>
#define FNV1A_64_INIT 0xcbf29ce484222325ULL
#define FNV1A_64_PRIME 0x100000001b3
#include <cstdint>
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
// 添加用户信息结构体：
struct UserInfo {
    string senderID;      // 发送方学号
    string senderName;    // 发送方姓名
    string receiverID;    // 接收方学号
    string receiverName;  // 接收方姓名
};
class HuffmanCompression {
private:
    vector<HuffmanNode*> heap;
    map<unsigned char, string> huffmanCodes; // 存储每个字节的哈夫曼编码

    // 堆的比较函数：词频小的优先，词频相同时按字节值排序
    static bool compareNodes(HuffmanNode* a, HuffmanNode* b) {
        if (a->frequency != b->frequency) {
            return a->frequency < b->frequency;  // 词频小的优先
        }
        // 词频相同时，按字节值排序
        return a->byte < b->byte;
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

    // 添加FNV-1a哈希计算函数
    uint64_t fnv1a_64(const void *data, size_t length) {
        uint64_t hash = FNV1A_64_INIT;
        const uint8_t *byte_data = (const uint8_t *)data;
        for (size_t i = 0; i < length; i++) {
            hash ^= byte_data[i];
            hash *= FNV1A_64_PRIME;
        }
        return hash;
    }
    // 计算文件的hash值
    uint64_t calculateFileHash(const string& filename) {
        ifstream file(filename, ios::binary);
        if (!file) {
            cerr << "无法打开文件进行哈希计算：" << filename << endl;
            return 0;
        }

        // 获取文件大小
        file.seekg(0, ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, ios::beg);

        // 读取文件内容
        vector<uint8_t> buffer(fileSize);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        file.close();

        // 计算哈希值
        return fnv1a_64(buffer.data(), fileSize);
    }

    // 将二进制位串写入文件
    void writeBitsToFile(const string& bits, ofstream& outFile) {
        for (size_t i = 0; i < bits.length(); i += 8) {
            string byte = bits.substr(i, std::min<size_t>(8ul, bits.length() - i));
            while (byte.length() < 8) {
                byte += '0';  // 在末尾补0
            }
            unsigned char value = static_cast<unsigned char>(stoi(byte, nullptr, 2));
            outFile.write(reinterpret_cast<const char*>(&value), 1);
        }
    }

    void siftUp(int index) {
        while (index > 0) {
            int parent = (index - 1) / 2;
            if (compareNodes(heap[index], heap[parent])) {
                swap(heap[index], heap[parent]);
                index = parent;
            } else {
                break;
            }
        }
    }

    // 修改插入节点的方法
    void insertNode(HuffmanNode* node) {
        heap.push_back(node);
        siftUp(heap.size() - 1);
    }

    vector<unsigned char> userInfoToBytes(const UserInfo& info) {
        vector<unsigned char> bytes;
        
        // 添加每个字段的长度和内容
        for (const string& field : {info.senderID, info.senderName, 
                                info.receiverID, info.receiverName}) {
            uint16_t length = static_cast<uint16_t>(field.length());
            bytes.push_back(length & 0xFF);
            bytes.push_back((length >> 8) & 0xFF);
            
            for (char c : field) {
                bytes.push_back(static_cast<unsigned char>(c));
            }
        }
        
        return bytes;
    }

    // 显示文件统计信息
    void displayFileStats(const string& filename, const vector<pair<unsigned char, int>>& frequencies) {
        cout << "\n处理后文件大小: " << getFileSize(filename) << " 字节" << endl;
        cout << "不同字符数量: " << frequencies.size() << endl << endl;
        printFrequencyStats(frequencies);
    }
    // 显示压缩统计信息
    void displayCompressionStats(const string& filename, int wpl) {
        cout << "\n哈夫曼树的WPL值：" << wpl << endl;
        double compressionRatio = calculateCompressionRatio(filename, wpl);
        cout << "预计压缩率：" << fixed << setprecision(2) << compressionRatio << "%" << endl;
    }
public:

    // 在原文件前添加用户信息并返回新文件路径
    string addUserInfoToFile(const string& inputFilename, const UserInfo& userInfo) {
        // 创建临时文件名
        string newFilename = inputFilename;
        size_t lastDot = newFilename.find_last_of('.');
        if (lastDot != string::npos) {
            newFilename = newFilename.substr(0, lastDot) + "_added" + 
                         newFilename.substr(lastDot);
        } else {
            newFilename += "_added";
        }
    
        ofstream tempFile(newFilename, ios::binary);
        if (!tempFile) {
            cerr << "无法创建添加用户信息的文件" << endl;
            return "";
        }

        // 写入用户信息头部
        tempFile << "发送方学号: " << userInfo.senderID << "\n";
        tempFile << "发送方姓名: " << userInfo.senderName << "\n";
        tempFile << "接收方学号: " << userInfo.receiverID << "\n";
        tempFile << "接收方姓名: " << userInfo.receiverName << "\n";
        tempFile << "------------------------\n";

        // 复制原文件内容
        ifstream inFile(inputFilename, ios::binary);
        if (!inFile) {
            cerr << "无法打开源文件" << endl;
            tempFile.close();
            return "";
        }

        tempFile << inFile.rdbuf();
        
        inFile.close();
        tempFile.close();
        
        return newFilename;
    }

    // 整理code_bin.txt文件
    void formatCodeTable() {
        // 打开二进制输入文件
        ifstream inFile("code_bin.txt", ios::binary);
        if (!inFile) {
            cerr << "无法打开编码表文件" << endl;
            return;
        }
    
        // 打开文本输出文件
        ofstream outFile("code.txt");
        if (!outFile) {
            cerr << "无法创建格式化后的编码表文件" << endl;
            inFile.close();
            return;
        }
    
        // 读取前4个字节获取文件大小
        vector<unsigned char> sizeBytes(4);
        inFile.read(reinterpret_cast<char*>(sizeBytes.data()), 4);
        
        // 计算并写入文件大小
        long fileSize = 0;
        for (int i = 0; i < 4; i++) {
            fileSize |= (static_cast<long>(sizeBytes[i]) << (i * 8));
        }
        outFile << fileSize << endl;
    
        // 读取编码表内容
        while (!inFile.eof()) {
            // 读取字节值和编码长度
            unsigned char byte, length;
            if (!inFile.read(reinterpret_cast<char*>(&byte), 1) ||
                !inFile.read(reinterpret_cast<char*>(&length), 1)) {
                break;
            }
    
            // 计算需要读取的字节数
            size_t bytesNeeded = (length + 7) / 8;
            vector<unsigned char> codeBytes(bytesNeeded);
    
            // 读取编码字节
            if (!inFile.read(reinterpret_cast<char*>(codeBytes.data()), bytesNeeded)) {
                break;
            }
    
            // 输出字节值和编码长度
            outFile << "0x" << hex << uppercase << setw(2) << setfill('0') 
                    << static_cast<int>(byte) << " 0x"
                    << setw(2) << static_cast<int>(length);
    
            // 输出编码字节
            for (size_t i = 0; i < bytesNeeded; i++) {
                outFile << " 0x" << setw(2) << setfill('0') 
                       << static_cast<int>(codeBytes[i]);
            }
            outFile << endl;
        }
    
        inFile.close();
        outFile.close();
    }
    

    bool generateCompressedFile(const string& inputFilename) {
        // 构造输出文件名
        string outputFilename = inputFilename;
        size_t lastDot = outputFilename.find_last_of('.');
        if (lastDot != string::npos) {
            outputFilename = outputFilename.substr(0, lastDot);
        }
        outputFilename += ".hfm";

        ofstream outFile(outputFilename, ios::binary);
        if (!outFile) {
            cerr << "无法创建压缩文件：" << outputFilename << endl;
            return false;
        }
        // vector<unsigned char> userInfoBytes = userInfoToBytes(userInfo);
        // outFile.write(reinterpret_cast<const char*>(userInfoBytes.data()), 
        //              userInfoBytes.size());

        // 读取输入文件并编码
        ifstream inFile(inputFilename, ios::binary);
        if (!inFile) {
            cerr << "无法打开源文件：" << inputFilename << endl;
            return false;
        }

        
        // 2. 写入编码后的内容
        string encodedBits;
        char byte;
        while (inFile.get(byte)) {
            encodedBits += huffmanCodes[static_cast<unsigned char>(byte)];
            // 当累积的位数足够多时，写入文件
            if (encodedBits.length() >= 1024) {
                writeBitsToFile(encodedBits.substr(0, encodedBits.length() - encodedBits.length() % 8), outFile);
                encodedBits = encodedBits.substr(encodedBits.length() - encodedBits.length() % 8);
            }
        }

        // 写入剩余的位
        if (!encodedBits.empty()) {
            writeBitsToFile(encodedBits, outFile);
        }

        inFile.close();
        outFile.close();

        cout << "\n压缩文件已生成：" << outputFilename << endl;
        cout << "压缩文件大小：" << getFileSize(outputFilename) << " 字节" << endl;

        return true;
    }

    void generateCodeTable(const string& filename, HuffmanNode* root) {
        // 生成哈夫曼编码
        huffmanCodes.clear();
        generateCodes(root);

        // 创建编码表文件
        ofstream codeFile("code_bin.txt", ios::binary);
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

        formatCodeTable();
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
        
        // 计算哈希值
        uint64_t hash = fnv1a_64(encodedBytes.data(), encodedBytes.size());

        // 显示编码信息
        cout << "\n压缩编码信息：" << endl;
        cout << "总字节数: " << encodedBytes.size() << endl;
        cout << "压缩后FNV-1a哈希值: 0x" << hex << uppercase << hash << dec << endl;

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
        
        heap.clear();

        // 初始化堆
        for (const auto& pair : frequencies) {
            insertNode(new HuffmanNode(pair.first, pair.second));
        }

        // 构建哈夫曼树
        while (heap.size() > 1) {
            // 取出两个最小节点
            HuffmanNode* left = extractMin();
            HuffmanNode* right = extractMin();

            // 创建新的父节点，使用较大的字节值
            HuffmanNode* parent = new HuffmanNode(
                max(left->byte, right->byte),
                left->frequency + right->frequency
            );

            // 确保频率小的在左边，频率相同时字节值小的在左边
            if (left->frequency > right->frequency || 
                (left->frequency == right->frequency && left->byte > right->byte)) {
                swap(left, right);
            }

            parent->left = left;
            parent->right = right;

            // 使用上滤操作插入新节点
            insertNode(parent);
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

     // 添加公共接口用于单独计算文件哈希值
     uint64_t getFileHash(const string& filename) {
        return calculateFileHash(filename);
    }

    // 压缩处理的统一接口
    bool compressFile(const string& filename, const UserInfo& userInfo, bool is_encrypted = false){
    
        uint64_t origin_hash = getFileHash(filename);
        // 打印文件基本信息
        cout << "原始文件哈希值: 0x" << hex << uppercase << setfill('0') 
         << setw(16) << origin_hash << dec << endl;
        // 创建带有用户信息的新文件
        string processedFile = addUserInfoToFile(filename, userInfo);
        if (processedFile.empty()) {
            cout << "处理文件失败！" << endl;
            return false;
        }
        // 计算添加信息后的哈希值
        uint64_t processed_hash = getFileHash(processedFile);
        cout << "\n添加用户信息后的文件哈希值: 0x" << hex << uppercase << setfill('0') 
         << setw(16) << processed_hash << dec << endl;
        // 获取词频统计 用于处理后的文件
        vector<pair<unsigned char, int>> frequencies = getFrequenciesFromFile(processedFile);
    
        if (frequencies.empty()) {
            cout << "文件为空或无法读取！" << endl;
            remove(processedFile.c_str());
            return false;
        }
        displayFileStats(processedFile, frequencies);

        // 5. 构建哈夫曼树并生成编码
        HuffmanNode* root = buildHuffmanTree(frequencies);
        int wpl = calculateWPL(root);
        displayCompressionStats(processedFile, wpl);
        // 6. 生成编码表
        generateCodeTable(processedFile, root);
        displayCodeTable();
        // 7. 如果需要加密，在这里添加加密处理
        if (is_encrypted) {
            // TODO: 添加加密逻辑
            cout << "加密功能待实现..." << endl;
        }
        // 8. 压缩文件
        if (!generateCompressedFile(processedFile)) {
            cerr << "生成压缩文件失败！" << endl;
            remove(processedFile.c_str());
            return false;
        }
        // 9. 显示压缩结果
        encodeAndShowLast16Bytes(processedFile);
        // 10. 清理临时文件
        remove(processedFile.c_str());
        return true;
    }
};




// 使用示例
int main() {
    HuffmanCompression huffman;
    string filename;
    UserInfo userInfo;
    char choice;
    //1. 读取文件路径
    cout << "请输入要压缩的文件路径: ";
    std::getline(cin, filename);
  
    //2. 读取用户信息
    cout << "\n请输入发送方信息: " << endl;
    cout << "学号: ";
    getline(cin, userInfo.senderID);
    cout << "姓名: ";
    getline(cin, userInfo.senderName);

    cout << "\n请输入接收方信息: " << endl;
    cout << "学号: ";
    getline(cin, userInfo.receiverID);
    cout << "姓名: ";
    getline(cin, userInfo.receiverName);

    // 3. 询问是否需要加密
    cout << "\n是否需要加密压缩?(Y/N,默认N): ";
    choice = cin.get();
    bool is_encrypted = (toupper(choice) == 'Y');

    // 4. 执行压缩处理
    if (!huffman.compressFile(filename, userInfo, is_encrypted)) {
        return 1;
    }

    cout << "\n压缩处理完成!" << endl;
    return 0;
}