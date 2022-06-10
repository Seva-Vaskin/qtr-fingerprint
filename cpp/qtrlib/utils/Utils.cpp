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

} // namespace qtr