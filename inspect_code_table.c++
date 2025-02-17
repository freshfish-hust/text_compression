#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <vector>
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

int main() {
    // 打开二进制输入文件
    formatCodeTable();
}