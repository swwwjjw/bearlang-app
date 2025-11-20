#include <cmath>
#include <iostream>
#include <string>

int main() {
    std::ios_base::sync_with_stdio(false);
    double vr_1{};
    double vr_2{};
    std::cout << "Введите первое число:" << std::endl;
    std::cin >> vr_1;
    std::cout << "Введите второе число:" << std::endl;
    std::cin >> vr_2;
    std::cout << "Сумма:" << std::endl;
    std::cout << (vr_1 + vr_2) << std::endl;
    std::cout << "Произведение:" << std::endl;
    std::cout << (vr_1 * vr_2) << std::endl;
    if ((vr_2 == 0)) {
        std::cout << "На ноль делить нельзя" << std::endl;
    }
    else {
        std::cout << (vr_1 / vr_2) << std::endl;
    }
    return 0;
}
