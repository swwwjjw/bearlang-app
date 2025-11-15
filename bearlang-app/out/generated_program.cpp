#include <cmath>
#include <iostream>
#include <string>

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout << std::boolalpha;
    double a;
    double b;
    std::cout << "Введите первое число:" << std::endl;
    std::cin >> a;
    std::cout << "Введите второе число:" << std::endl;
    std::cin >> b;
    std::cout << "Сумма:" << std::endl;
    std::cout << (a + b) << std::endl;
    std::cout << "Произведение:" << std::endl;
    std::cout << (a * b) << std::endl;
    if ((b == 0)) {
        std::cout << "На ноль делить нельзя" << std::endl;
    }
    else {
        std::cout << (a / b) << std::endl;
    }
    return 0;
}
