#include <iostream> // some libs are included for test purpose only
#include <cmath>
#include <sstream>
#include <string>
#include "price4.hpp"

namespace utils
{
Price4::Price4(const std::string& str)
{
    // this implementation is kinda slow - better way to optimize?
    std::stringstream is(str);
    std::string integer, decimal;

    std::getline(is, integer, '.');
    std::getline(is, decimal, '.');
    while (decimal.size() < Price4::scale)
    {
        decimal += "0";
    }
    unscaled_ = std::stol(integer + decimal.substr(0, Price4::scale));
}

std::string Price4::to_str() const
{
    const std::string unscaled = std::to_string(unscaled_);
    const size_t num_digits = unscaled.size();
    const size_t pivot_idx = num_digits - static_cast<size_t>(Price4::scale);
    const std::string integer = unscaled.substr(0, pivot_idx);
    const std::string decimal = unscaled.substr(pivot_idx, num_digits);
    //std::cout << "integer = " << integer << ", decimal = " << decimal << std::endl;

    size_t last_pos = decimal.size();
    while (last_pos > 0)
    {
        if (decimal.at(--last_pos) != '0') { break; }
    }
    //std::cout << "last_pos = " << last_pos << std::endl;
    if (last_pos == 0)
    {
        return integer;
    }
    return integer + "." + decimal.substr(0, last_pos + 1);
}

bool operator==(const Price4& a, const Price4& b)
{
    return a.unscaled() == b.unscaled();
}

bool operator!=(const Price4& a, const Price4& b)
{
    return !(a == b);
}

bool operator<(const Price4& a, const Price4& b)
{
    return a.unscaled() < b.unscaled();
}

bool operator<=(const Price4& a, const Price4& b)
{
    return !(a > b);
}

bool operator>(const Price4& a, const Price4& b)
{
    return a.unscaled() > b.unscaled();
}

bool operator>=(const Price4& a, const Price4& b)
{
    return !(a < b);
}

} // namespace utils

// for test purpose
// int main()
// {
//     Price4 p1("139.96"), p2("139"), p3("139.01"), p4("139.00");
//     std::cout << p1.unscaled() << ", to_str = " << p1.to_str() << std::endl;
//     std::cout << p2.unscaled() << ", to_str = " << p2.to_str() << std::endl;
//     std::cout << p3.unscaled() << ", to_str = " << p3.to_str() << std::endl;
//     std::cout << p4.unscaled() << ", to_str = " << p4.to_str() << std::endl;

//     std::cout << "p1 < p2 ? " << (p1 < p2) << std::endl;
//     std::cout << "p1 >= p2 ? " << (p1 >= p2) << std::endl;
//     std::cout << "p3 > p4 ? " << (p3 > p4) << std::endl;
//     std::cout << "p3 <= p4 ? " << (p3 <= p4) << std::endl;
//     return 0;
// }
