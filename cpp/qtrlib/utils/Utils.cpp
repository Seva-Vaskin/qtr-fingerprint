#include "Utils.h"

namespace qtr {

bool endsWith(const std::string &a, const std::string &b) {
    return a.size() >= b.size() &&
           (0 == a.compare(a.size() - b.size(), b.size(), b));
}

void emptyArgument(const std::string &argument, const std::string &message) {
    if (argument.empty()) {
        LOG(ERROR) << message;
        exit(-1);
    }
}

int chexToInt(char letter) {
    return letter >= '0' && letter <= '9' ? letter - '0' : letter - 'a' + 10;
}

std::vector<std::string> findFiles(const std::filesystem::path &pathToDir, const std::string &extension) {
    std::vector<std::string> sdfFiles;
    for (const auto &entry: std::filesystem::recursive_directory_iterator(pathToDir)) {
        if (entry.path().extension() == extension) {
            LOG(INFO) << entry.path().string() << std::endl;
            sdfFiles.push_back(entry.path().string());
        }
    }
    return sdfFiles;
}

} // namespace qtr