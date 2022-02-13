#ifndef STOCK_H_
#define STOCK_H_

#include <nlohmann/json.hpp>

namespace stock
{
    // it is not good to hardcoded this
    enum stock_symbol
    {
        AAPL,
        MSFT
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(
        stock_symbol,
        {
            {AAPL, "AAPL"},
            {MSFT, "MSFT"}
        }
    )
} // namespace stock

#endif