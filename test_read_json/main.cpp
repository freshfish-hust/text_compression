#include <iostream>
#include <fstream>
#include <string>
#include <map>

std::map<std::string, std::string> loadConfig(const std::string& filename) {
    std::map<std::string, std::string> config;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "无法打开配置文件！" << std::endl;
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);
            config[key] = value;
        }
    }

    file.close();
    return config;
}

int main() {
    auto config = loadConfig("identity.txt");

    std::string system_id = "U202312345";
    std::string system_name = "xt";

    if (config["id"] == system_id && config["name"] == system_name) {
        std::cout << "学号和姓名匹配成功！" << std::endl;
    } else {
        std::cout << "学号和姓名不匹配！" << std::endl;
    }

    return 0;
}