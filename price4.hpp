#ifndef PRICE4_HPP_
#define PRICE4_HPP_

#include <nlohmann/json.hpp>
#include <string>

namespace utils
{
class Price4
{
public:
    const static int scale = 4;
    
    Price4() = default;
    explicit Price4(long unscaled) : unscaled_(unscaled) {}

    // convert from string
    explicit Price4(const std::string& str);

    long unscaled() const { return unscaled_; }

    // convert to string
    std::string to_str() const;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Price4, unscaled_);

private:
    long unscaled_;
};

bool operator==(const Price4& a, const Price4& b);
bool operator!=(const Price4& a, const Price4& b);

bool operator<(const Price4& a, const Price4& b);
bool operator<=(const Price4& a, const Price4& b);

bool operator>(const Price4& a, const Price4& b);
bool operator>=(const Price4& a, const Price4& b);

} // namespace utils

#endif