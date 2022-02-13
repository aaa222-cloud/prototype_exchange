#ifndef MATCHING_ENGINE_
#define MATCHING_ENGINE_

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <tuple>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "event.hpp"
#include "order.hpp"
#include "order_book.hpp"
#include "size_rules.hpp"
#include "stock.hpp"
#include "ticker_rules.hpp"

namespace exchange
{

using json = nlohmann::json;

class MatchingEngine;
typedef std::unique_ptr<MatchingEngine> MatchingEnginePtr;
typedef std::unique_ptr<const MatchingEngine> MatchingEngineCPtr;

class MatchingEngine
{
public:
    MatchingEngine() = default;
    MatchingEngine(
        const std::vector<order::LimitOrderPtr>& orders,
        const size_rules::TickSizeRulesCPtr& ticker_size_rules,
        const size_rules::LotSizeRulesCPtr& lot_size_rules,
        const ticker_rules::TickerRulesCPtr& ticker_rules
    );
    MatchingEngine(
        const std::vector<std::string>& orders,
        const size_rules::TickSizeRulesCPtr& ticker_size_rules,
        const size_rules::LotSizeRulesCPtr& lot_size_rules,
        const ticker_rules::TickerRulesCPtr& ticker_rules
    );

    bool validate_order(const std::string& o) const;
    bool validate_order(const order::OrderBasePtr& o) const;
    std::vector<trade_event::EventBaseCPtr> process_order(const std::string& s);
    std::vector<trade_event::EventBaseCPtr> process_order(order::OrderBasePtr& o);

    std::vector<std::string> eod_cleanup();

private:
    void initialise(const std::vector<order::OrderBaseCPtr>& orders);

    std::vector<trade_event::EventBaseCPtr> cancel_order(int order_id);
    std::vector<trade_event::EventBaseCPtr> insert_order(order::LimitOrderPtr& o);
    std::vector<trade_event::EventBaseCPtr> match_order(order::OrderBasePtr& o);

    typedef std::tuple<stock::stock_symbol, order::order_side> book_key_type;
    std::unordered_map<std::string, order::OrderBookPtr> order_books_;
    // pointers to size rules
    size_rules::TickSizeRulesCPtr ticker_size_rules_;
    size_rules::LotSizeRulesCPtr lot_size_rules_;
    ticker_rules::TickerRulesCPtr ticker_rules_;
};

} // namespace exchange

#endif
