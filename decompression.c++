#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
using namespace std;

// 定义解码树节点结构
struct DecodeNode {
    unsigned char byte;     // 字节值
    bool isLeaf;           // 是否是叶子节点
    DecodeNode* left;      // 左子节点 (0)
    DecodeNode* right;     // 右子节点 (1)
    
    DecodeNode() : byte(0), isLeaf(false), left(nullptr), right(nullptr) {}
    DecodeNode(unsigned char b) : byte(b), isLeaf(true), left(nullptr), right(nullptr) {}
};

class HuffmanDecompression {
private:
    DecodeNode* root;      // 解码树根节点

    // 从编码表构建解码树
    void buildDecodeTree(unsigned char byte, const string& code) {
        DecodeNode* current = root;
        
        for (size_t i = 0; i < code.length(); i++) {
            if (code[i] == '0') {
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
    void cleanupTree(DecodeNode* node) {
        if (node) {
            cleanupTree(node->left);
            cleanupTree(node->right);
            delete node;
        }
    }

    // 从字节还原二进制串
    string byteToBinaryString(unsigned char byte, unsigned char validBits = 8) {
        string binary;
        for (int i = 7; i >= 8 - validBits; i--) {
            binary += ((byte >> i) & 1) ? '1' : '0';
        }
        return binary;
    }

public:
    HuffmanDecompression() : root(new DecodeNode()) {}
    ~HuffmanDecompression() {
        cleanupTree(root);
    }

    // 读取编码表并构建解码树
    bool loadCodeTable(const string& codeTablePath) {
        ifstream codeFile(codeTablePath, ios::binary);
        if (!codeFile) {
            cerr << "无法打开编码表文件！" << endl;
            return false;
        }

        // 跳过文件大小信息（4字节）
        codeFile.seekg(4);

        // 读取编码表
        while (codeFile.peek() != EOF) {
            unsigned char byte, codeLength;
            
            // 读取字节值和编码长度
            codeFile.read(reinterpret_cast<char*>(&byte), 1);
            codeFile.read(reinterpret_cast<char*>(&codeLength), 1);
            
            if (codeFile.eof()) break;
    
            // 读取编码字节数据
            size_t bytesNeeded = (codeLength + 7) / 8;//根据编码长度计算所需字节数
            vector<unsigned char> codeBytes(bytesNeeded);
            codeFile.read(reinterpret_cast<char*>(codeBytes.data()), bytesNeeded);
    
            // 将编码字节转换为二进制字符串
            string code;
            for (size_t i = 0; i < bytesNeeded; i++) {
                unsigned char validBits = (i == bytesNeeded - 1 && codeLength % 8) ? 
                                        (codeLength % 8) : 8;
                string binStr = byteToBinaryString(codeBytes[i], validBits);
                code += binStr;
            }
    
            // 确保编码长度正确
            code = code.substr(0, codeLength);
    
            cout << "读取编码 - 字节: 0x" << hex << static_cast<int>(byte) 
                 << " 长度: " << dec << static_cast<int>(codeLength) 
                 << " 编码: " << code << endl;
    
            // 构建解码树
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

        // 读取原文件大小
        long originalSize;
        inFile.read(reinterpret_cast<char*>(&originalSize), sizeof(originalSize));

        // 构造输出文件名
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

        // 解码并写入文件
        DecodeNode* current = root;
        long decodedSize = 0;
        char byte;
        
        while (inFile.get(byte) && decodedSize < originalSize) {
            string binStr = byteToBinaryString(static_cast<unsigned char>(byte));
            
            for (char bit : binStr) {
                if (decodedSize >= originalSize) break;
                
                if (!current) {
                    cerr << "解码树错误！" << endl;
                    return false;
                }

                current = (bit == '0') ? current->left : current->right;
                
                if (current && current->isLeaf) {
                    outFile.put(current->byte);
                    decodedSize++;
                    current = root;
                }
            }
        }

        inFile.close();
        outFile.close();
        // 在解压函数中添加调试输出
        cout << "原文件大小: " << originalSize << " 字节" << endl;
        cout << "已解码大小: " << decodedSize << " 字节" << endl;
        cout << "解压完成！文件已保存为：" << outputPath << endl;
        return true;
    }
};

int main() {
    HuffmanDecompression decompressor;
    string compressedFile;
    
    cout << "请输入压缩文件路径(.hfm): ";
    getline(cin, compressedFile);

    // 构造编码表文件路径
    string codeTableFile = "code.txt";

    // 加载编码表并构建解码树
    if (!decompressor.loadCodeTable(codeTableFile)) {
        return 1;
    }

    // 解压文件
    if (!decompressor.decompress(compressedFile)) {
        return 1;
    }

    return 0;
}