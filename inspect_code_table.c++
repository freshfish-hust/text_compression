#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <vector>
#include <sstream>
using namespace std;

string byteToBinary(unsigned char byte) {
    string binary;
    for (int i = 7; i >= 0; i--) {
        binary += ((byte >> i) & 1) ? '1' : '0';
    }
    return binary;
}

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

int hexStringToInt(const string& hexStr) {
    return stoi(hexStr.substr(2), nullptr, 16);  // 跳过"0x"前缀
}

bool convertToBinary() {
    ifstream inFile("code.txt");
    if (!inFile) {
        cerr << "无法打开文本编码表文件" << endl;
        return false;
    }

    // 打开二进制输出文件
    ofstream outFile("code_bin.txt", ios::binary);
    if (!outFile) {
        cerr << "无法创建二进制编码表文件" << endl;
        inFile.close();
        return false;
    }

    // 读取第一行的文件大小（十进制）
    string fileSizeStr;
    getline(inFile, fileSizeStr);
    uint32_t fileSize = stoul(fileSizeStr);  // 使用无符号32位整数

    // 将文件大小转换为4个字节写入
    for (int i = 3; i >= 0; i--) {  // 从高字节到低字节
        unsigned char sizeByte = (fileSize >> (i * 8)) & 0xFF;
        outFile.put(sizeByte);
    }

    // 读取并处理剩余的编码表内容
    string line;
    while (getline(inFile, line)) {
        if (line.empty()) continue;

        // 移除可能的回车符
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        stringstream ss(line);
        string byteHex, lengthHex, codeHex;
        
        // 读取字节值和编码长度
        ss >> byteHex >> lengthHex;
        
        // 移除"0x"前缀并转换为数值
        unsigned char byte = stoi(byteHex.substr(2), nullptr, 16);
        unsigned char length = stoi(lengthHex.substr(2), nullptr, 16);
        
        // 写入字节值和长度
        outFile.put(byte);
        outFile.put(length);

        // 读取并写入编码字节
        while (ss >> codeHex) {
            unsigned char codeByte = stoi(codeHex.substr(2), nullptr, 16);
            outFile.put(codeByte);
        }
    }

    inFile.close();
    outFile.close();
    return true;
}

int main() {
    //1. 打印原始 code_bin.txt 的内容
    cout << "原始 code_bin.txt 的内容：" << endl;
    ifstream originalFile("code_bin.txt", ios::binary);
    if (originalFile) {
        char byte;
        while (originalFile.get(byte)) {
            cout << byteToBinary(static_cast<unsigned char>(byte));
        }
        cout << endl;
        originalFile.close();
    } else {
        cout << "未找到原始的 code_bin.txt 文件" << endl;
    }

    // 2. 将 code.txt 转换为 code_bin.txt
    if (!convertToBinary()) {
        cerr << "转换失败！" << endl;
        return 1;
    }

    // 3. 打印转换后的 code_bin.txt 的内容
    cout << "\n转换后的 code_bin.txt 的内容：" << endl;
    ifstream convertedFile("code_bin.txt", ios::binary);
    if (!convertedFile) {
        cerr << "无法打开转换后的文件！" << endl;
        return 1;
    }

    char convertedByte;
    while (convertedFile.get(convertedByte)) {
        cout << byteToBinary(static_cast<unsigned char>(convertedByte));
    }
    cout << endl;
    
    convertedFile.close();
    return 0;
}