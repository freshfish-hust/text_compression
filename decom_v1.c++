#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <chrono>
#include <iomanip>
#define FNV1A_64_INIT 0xcbf29ce484222325ULL
#define FNV1A_64_PRIME 0x100000001b3
using namespace std;
using namespace chrono;

// 解码树节点结构
struct DecodeNode {
    unsigned char byte;
    bool isLeaf;
    DecodeNode* left;
    DecodeNode* right;
    
    DecodeNode() : byte(0), isLeaf(false), left(nullptr), right(nullptr) {}
    DecodeNode(unsigned char b) : byte(b), isLeaf(true), left(nullptr), right(nullptr) {}
};

class HuffmanDecompression {
private:
    DecodeNode* root;
    long originalFileSize;

    uint64_t fnv1a_64(const void *data, size_t length) {
        uint64_t hash = FNV1A_64_INIT;
        const uint8_t *byte_data = (const uint8_t *)data;
        for (size_t i = 0; i < length; i++) {
            hash ^= byte_data[i];
            hash *= FNV1A_64_PRIME;
        }
        return hash;
    }
    //计算文件的hash值
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

    // 将十六进制字符串转换为整数
    int hexStringToInt(const string& hexStr) {
        return stoi(hexStr.substr(2), nullptr, 16);
    }

    // 将字节转换为二进制字符串
    string byteToBinaryString(unsigned char byte, unsigned char validBits = 8) {
        string binary;
        for (int i = 7; i >= 8 - validBits; i--) {
            binary += ((byte >> i) & 1) ? '1' : '0';
        }
        return binary;
    }

    // 构建解码树
    void buildDecodeTree(unsigned char byte, const string& code) {
        if (!root) root = new DecodeNode();
        
        DecodeNode* current = root;
        for (char bit : code) {
            if (bit == '0') {
                if (!current->left) {
                    current->left = new DecodeNode();
                }
                current = current->left;
            } else {
                if (!current->right) {
                    current->right = new DecodeNode();
                }
                current = current->right;
            }
        }
        current->byte = byte;
        current->isLeaf = true;
    }

    // 清理解码树
    void clearDecodeTree(DecodeNode* node) {
        if (node) {
            clearDecodeTree(node->left);
            clearDecodeTree(node->right);
            delete node;
        }
    }

public:
    HuffmanDecompression() : root(nullptr) {}
    ~HuffmanDecompression() {
        clearDecodeTree(root);
    }

    // 加载编码表
    bool loadCodeTable(const string& filename) {
        ifstream codeFile(filename);
        if (!codeFile) {
            cerr << "无法打开编码表文件！" << endl;
            return false;
        }

        // 读取文件大小
        string fileSizeStr;
        getline(codeFile, fileSizeStr);
        originalFileSize = stol(fileSizeStr);
        cout << "原文件大小: " << originalFileSize << " 字节" << endl;

        // 读取编码表
        string line;
        while (getline(codeFile, line)) {
            if (line.empty()) continue;

            stringstream ss(line);
            string byteHex, lengthHex;
            vector<string> codeHexBytes;

            ss >> byteHex >> lengthHex;
            unsigned char byte = hexStringToInt(byteHex);
            unsigned char length = hexStringToInt(lengthHex);

            string code;
            string codeHex;
            while (ss >> codeHex) {
                unsigned char codeByte = hexStringToInt(codeHex);
                code += byteToBinaryString(codeByte);
            }

            // 确保编码长度正确
            code = code.substr(0, length);
            buildDecodeTree(byte, code);
        }

        codeFile.close();
        return true;
    }

    // 解压缩文件
    bool decompress(const string& compressedPath) {
        ifstream inFile(compressedPath, ios::binary);
        if (!inFile) {
            cerr << "无法打开压缩文件！" << endl;
            return false;
        }

        string outputPath = compressedPath;
        size_t lastDot = outputPath.find_last_of('.');
        if (lastDot != string::npos) {
            outputPath = outputPath.substr(0, lastDot);
        }
        outputPath += "_j.txt";

        ofstream outFile(outputPath, ios::binary);
        if (!outFile) {
            cerr << "无法创建解压文件！" << endl;
            return false;
        }

        auto startTime = high_resolution_clock::now();

        DecodeNode* current = root;
        long decodedSize = 0;
        char byte;
        unsigned char bitPos = 7;

        while (inFile.get(byte) && decodedSize < originalFileSize) {
            unsigned char curByte = static_cast<unsigned char>(byte);
            
            do {
                bool bit = (curByte >> bitPos) & 1;
                current = bit ? current->right : current->left;
                
                if (current && current->isLeaf) {
                    outFile.put(current->byte);
                    decodedSize++;
                    current = root;
                }
                
                if (bitPos == 0) {
                    bitPos = 7;
                    break;
                }
                bitPos--;
            } while (decodedSize < originalFileSize);
        }

        auto endTime = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(endTime - startTime);
        
        

        cout << "\n解压缩统计信息：" << endl;
        cout << "----------------------------------------" << endl;
        cout << "解压耗时: " << fixed << setprecision(6) 
             << duration.count() / 1000000.0 << " 秒" << endl;
        cout << "已解码大小: " << decodedSize << " 字节" << endl;
        cout << "----------------------------------------" << endl;

        inFile.close();
        outFile.close();
        uint64_t decompressedHash = calculateFileHash(outputPath);
        cout << "解压文件哈希值: 0x" << hex << uppercase << setfill('0') 
             << setw(16) << decompressedHash << dec << endl;
        return true;
    }
};

int main() {
    HuffmanDecompression decompressor;
    string compressedFile;
    
    cout << "请输入压缩文件路径(.hfm): ";
    getline(cin, compressedFile);

    // 加载编码表并解压
    if (!decompressor.loadCodeTable("code.txt") || 
        !decompressor.decompress(compressedFile)) {
        return 1;
    }

    return 0;
}