#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.starts_with("protocol_version"))
            std::cout << "= 2\n\n";
        else if (line.starts_with("genmove")) {
            if (const auto delay = std::getenv("WEIQI_GTP_TEST_DELAY_MS"))
                std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(delay)));
            const auto move = std::getenv("WEIQI_GTP_TEST_MOVE");
            std::cerr << "fixture diagnostic on stderr\n";
            std::cout << "= " << (move ? move : "D4") << "\n\n";
        } else
            std::cout << "=\n\n";
        std::cout.flush();
    }
}
