#include <iostream> // some libs are included for test purpose only
#include <cmath>
#include <sstream>
#include <string>
#include "price4.hpp"

namespace utils
{
Price4::Price4(const std::string& s)
{
    const long unit = std::pow(10, Price4::scale);

    const size_t num_chars = s.size();
    const size_t pivot_idx = s.find(".");
    if (pivot_idx == num_chars)
    {
        unscaled_ = std::stol(s) * unit;
    } 
    else
    {
        unscaled_ = std::stol(s.substr(0, pivot_idx)) * unit;

        // there is decimal part left
        if (pivot_idx < num_chars - 1)
        {
            const std::string decimal_s = s.substr(pivot_idx + 1, num_chars - pivot_idx);
            int num_digits = static_cast<int>(decimal_s.size());
            long multiplier = 1;
            while (num_digits < Price4::scale) { multiplier *= 10; ++num_digits; }
            unscaled_ += std::stol(decimal_s) * multiplier;
        }
    }
}

std::string Price4::to_str() const
{
    const long unit = std::pow(10, Price4::scale);

    long integer = unscaled_ / unit;
    long decimal = unscaled_ % unit;
    if (decimal == 0) { return std::to_string(integer); }

    std::string integer_s = std::to_string(integer) + ".";
    // deal with leading 0s in decimal part
    while (decimal * 10 < unit)
    {
        integer_s += "0";
        decimal *= 10;
    }

    // trim 0s at decimal part end
    decimal = unscaled_ % unit;
    while (decimal % 10 == 0)
    {
        decimal /= 10;
    }
    
    return  integer_s + std::to_string(decimal);
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
