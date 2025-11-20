#include <cmath>
#include <iostream>
#include <string>

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout << std::boolalpha;
    int счетчик = 0;
    while ((счетчик < 3)) {
        std::cout << "Готовимся" << std::endl;
        счетчик = (счетчик + 1);
    }
    for (int i = 1; i <= 5; ++i) {
        std::cout << i << std::endl;
    }
    return 0;
}
