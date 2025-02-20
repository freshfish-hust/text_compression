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
// 用户信息
struct UserInfo {
    string senderID;
    string senderName;
    string receiverID;
    string receiverName;
};
// 解码树节点结构
struct DecodeNode {
    unsigned char byte;
    bool isLeaf;
    DecodeNode* left;
    DecodeNode* right;
    
    DecodeNode() : byte(0), isLeaf(false), left(nullptr), right(nullptr) {}
    DecodeNode(unsigned char b) : byte(b), isLeaf(true), left(nullptr), right(nullptr) {}
};
enum class EncryptionType {
    NONE,
    OFFSET,
    XOR_KEY
};
const string DEFAULT_KEY = "HuffmanSecretKey2024";  // 可以根据需要修改默认密钥
class HuffmanDecompression {
private:
    DecodeNode* root;
    long originalFileSize;
    UserInfo userInfo;
    static const uint8_t OFFSET_VALUE = 0x55;
    static const int ID_LENGTH = 10;        // 学号固定长度
    static const int MAX_NAME_LENGTH = 20;  // 姓名最大长度
    //解密单个字节
    char decryptByte(char byte,EncryptionType type, const string& key = "", size_t keyIndex = 0) {
        switch(type) {
            case EncryptionType::OFFSET:
                return byte - OFFSET_VALUE;
            case EncryptionType::XOR_KEY:
                return byte ^ key[keyIndex % key.length()];
            default:
                return byte;
        }
    }
    //解密文件内容
    // bool decryptContent(const string& inputPath, const string& outputPath) {
    //     ifstream inFile(inputPath, ios::binary);
    //     ofstream outFile(outputPath, ios::binary);
        
    //     if (!inFile || !outFile) {
    //         cerr << "打开文件失败" << endl;
    //         return false;
    //     }

    //     char byte;
    //     while (inFile.get(byte)) {
    //         outFile.put(decryptByte(byte));
    //     }

    //     inFile.close();
    //     outFile.close();
    //     return true;
    // }
    // 解密并验证用户信息
    bool readAndVerifyUserInfo(ifstream& file, DecodeNode* root, EncryptionType encType, const string& key = "") {
        string decodedInfo;
        DecodeNode* current = root;
        char byte;
        int newlineCount = 0;
        size_t keyIndex = 0;
        while (file.get(byte)) {

            unsigned char curByte = static_cast<unsigned char>(byte);
            for (int bitPos = 7; bitPos >= 0; bitPos--) {
                bool bit = (curByte >> bitPos) & 1;
                current = bit ? current->right : current->left;

                if (current && current->isLeaf) {
                    char decodedChar = current->byte;
                    if (encType != EncryptionType::NONE) {
                    decodedChar = decryptByte(decodedChar,encType, key, keyIndex++);
                }
                    decodedInfo += decodedChar;
                    current = root;

                    if (decodedChar == '\n') {
                        newlineCount++;
                        if (newlineCount >= 4) {
                            size_t pos = decodedInfo.find("------------------------");
                            if (pos != string::npos) {
                                decodedInfo = decodedInfo.substr(0, pos + 24);
                                return parseUserInfo(decodedInfo);
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

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

    // 从identity.txt读取接收者信息
    bool loadIdentityInfo(string& id, string& name) {
        const int ID_LENGTH = 10;        // 学号固定长度
        const int MAX_NAME_LENGTH = 20;  // 姓名最大长度
    
        ifstream idFile("identity.txt");
        if (!idFile) {
            cerr << "无法打开身份配置文件 identity.txt" << endl;
            return false;
        }

        string line;
        bool hasId = false, hasName = false;
        
        while (getline(idFile, line)) {
            if (line.substr(0, 3) == "id=") {
                id = line.substr(3);
                // 验证学号长度
                if (id.length() != ID_LENGTH) {
                    cerr << "错误：学号必须是" << ID_LENGTH << "位！当前长度：" 
                         << id.length() << endl;
                    idFile.close();
                    return false;
                }
                hasId = true;
            } 
            else if (line.substr(0, 5) == "name=") {
                name = line.substr(5);
                // 验证姓名长度
                if (name.empty() || name.length() > MAX_NAME_LENGTH) {
                    cerr << "错误：姓名长度必须在1-" << MAX_NAME_LENGTH << "个字符之间！当前长度：" 
                         << name.length() << endl;
                    idFile.close();
                    return false;
                }
                hasName = true;
            }
        }
        
        idFile.close();
        
        if (!hasId || !hasName) {
            cerr << "错误：identity.txt 文件格式不正确！" << endl;
            cerr << "必须包含 id=和name=行" << endl;
            return false;
        }
        
        return true;
    }


    // 验证用户身份
    bool verifyIdentity() {
        string configID, configName;
        if (!loadIdentityInfo(configID, configName)) {
            return false;
        }

        if (userInfo.receiverID != configID || userInfo.receiverName != configName) {
            cerr << "\n身份验证失败！" << endl;
            cerr << "压缩文件中的接收者信息：" << endl;
            cerr << "学号: " << userInfo.receiverID << endl;
            cerr << "姓名: " << userInfo.receiverName << endl;
            cerr << "\n本地配置的身份信息：" << endl;
            cerr << "学号: " << configID << endl;
            cerr << "姓名: " << configName << endl;
            return false;
        }
        return true;
    }

    bool parseUserInfo(const string& decodedInfo){
        stringstream userInfoStream(decodedInfo);
        string line;
        int count = 0;

        while (count < 5 && getline(userInfoStream, line)) {
            if (count == 0 && line.find("发送方学号: ") == 0) {
                userInfo.senderID = line.substr(17);  // 17是"发送方学号: "的长度
                cout<<"senderID:"<<userInfo.senderID<<endl;
                cout << "发送方学号长度: " << userInfo.senderID.length() << endl;  // 调试输出
                if (userInfo.senderID.length() != ID_LENGTH) {
                    cerr << "发送方学号长度错误，当前长度：" << userInfo.senderID.length() 
                         << "，期望长度：" << ID_LENGTH << endl;
                    return false;
                }
                count++;
            }
            else if (count == 1 && line.find("发送方姓名: ") == 0) {
                userInfo.senderName = line.substr(17);
                if (userInfo.senderName.empty() || userInfo.senderName.length() > MAX_NAME_LENGTH) {
                    cerr << "发送方姓名长度错误" << endl;
                    return false;
                }
                count++;
            }
            else if (count == 2 && line.find("接收方学号: ") == 0) {
                userInfo.receiverID = line.substr(17);
                if (userInfo.receiverID.length() != ID_LENGTH) {
                    cerr << "接收方学号长度错误" << endl;
                    return false;
                }
                count++;
            }
            else if (count == 3 && line.find("接收方姓名: ") == 0) {
                userInfo.receiverName = line.substr(17);
                if (userInfo.receiverName.empty() || userInfo.receiverName.length() > MAX_NAME_LENGTH) {
                    cerr << "接收方姓名长度错误" << endl;
                    return false;
                }
                count++;
            }
            else if (line.find("------------------------") == 0) {
                count++;
            }
        }

        if (count < 5) {
            cerr << "用户信息不完整" << endl;
            return false;
        }
        // 进行身份验证
        // 验证身份
        if (!verifyIdentity()) {
            cerr << "身份验证失败，停止解压！" << endl;
            return false;
        }

        return true;
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
    bool decompress(const string& compressedPath, EncryptionType encType = EncryptionType::NONE,
        const string& key = DEFAULT_KEY) {
        ifstream inFile(compressedPath, ios::binary);
        if (!inFile) {
            cerr << "无法打开压缩文件！" << endl;
            return false;
        }
        // 验证用户信息
        if (!readAndVerifyUserInfo(inFile, root, encType, key)) {
            cerr << "用户信息验证失败！" << endl;
            return false;
        }

        cout << "\n身份验证成功！" << endl;
        cout << "发送方信息：" << userInfo.senderID << " - " << userInfo.senderName << endl;
        cout << "接收方信息：" << userInfo.receiverID << " - " << userInfo.receiverName << endl;
        
        inFile.seekg(0);//将文件指针移动到文件开头
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
        size_t keyIndex = 0;
        while (inFile.get(byte) && decodedSize < originalFileSize) {

            // 如果是加密文件，先解密字节
            unsigned char curByte = static_cast<unsigned char>(byte);
            
            do {
                bool bit = (curByte >> bitPos) & 1;
                current = bit ? current->right : current->left;
                
                if (current && current->isLeaf) {
                    char decodedChar = current->byte;
                    if (encType != EncryptionType::NONE) {
                    decodedChar = decryptByte(decodedChar,encType, key, keyIndex++);
                    }
                    outFile.put(decodedChar);
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
    string customKey;
    
    cout << "请输入压缩文件路径(.hfm): ";
    getline(cin, compressedFile);

    cout << "请选择解密方式：" << endl;
    cout << "0. 未加密文件" << endl;
    cout << "1. 偏移加密" << endl;
    cout << "2. XOR密钥加密" << endl;
    cout << "请输入选择 (0-2): ";
    
    int encChoice;
    cin >> encChoice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    EncryptionType encType = EncryptionType::NONE;
    string key = DEFAULT_KEY;

    switch (encChoice) {
        case 1:
            encType = EncryptionType::OFFSET;
            cout << "使用偏移解密方式" << endl;
            break;
        case 2:
            encType = EncryptionType::XOR_KEY;
            cout << "是否使用自定义密钥？(Y/N，默认使用内置密钥): ";
            if (toupper(cin.get()) == 'Y') {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "请输入密钥: ";
                getline(cin, customKey);
                key = customKey;
            }
            break;
        default:
            encType = EncryptionType::NONE;
            cout << "不进行解密" << endl;
    }
    // 加载编码表并解压
    if (!decompressor.loadCodeTable("code.txt") || 
        !decompressor.decompress(compressedFile, encType, key)) {
        return 1;
    }
    cout<<"解压成功！"<<endl;
    return 0;
}