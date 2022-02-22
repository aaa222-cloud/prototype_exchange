#include <string>
#include "ticker_rules.hpp"

namespace ticker_rules
{

TickerRules::TickerRules(
    const std::vector<std::string>& tickers
)
:
valid_symbols_(tickers.begin(), tickers.end())
{}

bool TickerRules::is_valid(const std::string& symbol) const
{
    return valid_symbols_.count(symbol) > 0;
}

}