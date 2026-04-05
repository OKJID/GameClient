#include <iostream>
#include <string>
#include <map>

int main() {
    std::multimap<std::string, std::string> m;
    m.insert(m.end(), std::make_pair("foo", "A"));
    m.insert(m.end(), std::make_pair("foo", "B"));
    
    auto range = m.equal_range("foo");
    for (auto it = range.first; it != range.second; ++it) {
        std::cout << it->second << std::endl;
    }
    return 0;
}
