#include "ticker_rules.hpp"

namespace ticker_rules
{

TickerRules::TickerRules(
    const std::vector<stock::stock_symbol>& tickers
)
:
valid_symbols_(tickers.begin(), tickers.end())
{}

bool TickerRules::is_valid(stock::stock_symbol symbol) const
{
    return valid_symbols_.count(symbol) > 0;
}

}