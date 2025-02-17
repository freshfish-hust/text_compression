#include <iostream>
#include <fstream>
#include <string>
using namespace std;

string byteToBinary(unsigned char byte) {
    string binary;
    for (int i = 7; i >= 0; i--) {
        binary += ((byte >> i) & 1) ? '1' : '0';
    }
    return binary;
}

int main() {

    long fileSize = 4;
    printf("fileSize:%ld\n",fileSize);
    ifstream file("code.txt", ios::binary);
    if (!file) return 1;

    char byte;
    while (file.get(byte)) {
        cout << byteToBinary(static_cast<unsigned char>(byte));
    }
    
    file.close();
    return 0;
}