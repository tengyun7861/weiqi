#include <iostream>
#include <string>

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.starts_with("protocol_version")) std::cout << "= 2\n\n";
        else if (line.starts_with("genmove")) std::cout << "= D4\n\n";
        else std::cout << "=\n\n";
        std::cout.flush();
    }
}
