#ifndef PRICE4_HPP_
#define PRICE4_HPP_

#include <functional>
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

private:
    // function for serialise
    template <typename BasicJsonType>
    friend void to_json(BasicJsonType& j, const Price4& p);
    template <typename BasicJsonType>
    friend void from_json(const BasicJsonType& j, Price4& p);

    long unscaled_;
};

bool operator==(const Price4& a, const Price4& b);
bool operator!=(const Price4& a, const Price4& b);

bool operator<(const Price4& a, const Price4& b);
bool operator<=(const Price4& a, const Price4& b);

bool operator>(const Price4& a, const Price4& b);
bool operator>=(const Price4& a, const Price4& b);

template <typename BasicJsonType>
void to_json(BasicJsonType& j, const Price4& p)
{
    j = p.to_str();
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, Price4& p)
{
    p = Price4(j.template get<std::string>());
}

} // namespace utils

namespace std
{
template <>
struct hash<const utils::Price4>
{
    size_t operator()(const utils::Price4& p) const noexcept
    {
        return std::hash<std::string>{}(p.to_str());
    }
};

} // namespace std

#endif