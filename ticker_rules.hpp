#ifndef TICKER_RULES_H_
#define TICKER_RULES_H_

#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_set>
#include <vector>
#include "stock.hpp"

namespace ticker_rules
{

using json = nlohmann::json;

class TickerRules;
typedef std::shared_ptr<TickerRules> TickerRulesPtr;
typedef std::shared_ptr<const TickerRules> TickerRulesCPtr;

class TickerRules
{
public:
    TickerRules() = default;
    TickerRules(const std::vector<stock::stock_symbol>& tickers);

    bool is_valid(stock::stock_symbol symbol) const;

private:
    // function for serialise
    template <typename BasicJsonType>
    friend void to_json(BasicJsonType& j, const TickerRules& o);
    template <typename BasicJsonType>
    friend void from_json(const BasicJsonType& j, TickerRules& o);

    std::unordered_set<stock::stock_symbol> valid_symbols_;
};

template <typename BasicJsonType>
void to_json(BasicJsonType& j, const TickerRules& o)
{
    j = BasicJsonType(o.valid_symbols_);
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, TickerRules& o)
{
    o = TickerRules(j.template get<std::vector<stock::stock_symbol>>());
}

} // namespace ticker_rules


#endif