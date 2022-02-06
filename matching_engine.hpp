#ifndef MATCHING_ENGINE_
#define MATCHING_ENGINE_

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <queue>
#include <unordered_map>
#include "event.hpp"
#include "order.hpp"
#include "order_book.hpp"
#include "stock.hpp"

namespace exchange
{

class MatchingEngine
{
public:
    MatchingEngine() = default;
    MatchingEngine(const std::vector<order::OrderBaseCPtr>& orders);
    MatchingEngine(const std::vector<std::string>& orders);

    std::vector<trade_event::EventBaseCPtr> process_order(const std::string& s);
    std::vector<trade_event::EventBaseCPtr> process_order(order::OrderBasePtr& o);

private:
    void initialise(const std::vector<order::OrderBaseCPtr>& orders);

    std::vector<trade_event::EventBaseCPtr> cancel_order(int order_id);
    std::vector<trade_event::EventBaseCPtr> insert_order(order::LimitOrderPtr& o);
    std::vector<trade_event::EventBaseCPtr> match_order(order::OrderBasePtr& o);

    std::unordered_map<std::pair<stock::stock_symbol, order::order_side>, order::OrderBookPtr> order_books_;
};

} // namespace exchange

#endif
