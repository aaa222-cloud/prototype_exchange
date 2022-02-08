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
#include "stock.hpp"

namespace exchange
{

class MatchingEngine
{
public:
    MatchingEngine() = default;
    MatchingEngine(const std::vector<order::LimitOrderPtr>& orders);
    MatchingEngine(const std::vector<std::string>& orders);

    std::vector<trade_event::EventBaseCPtr> process_order(const std::string& s);
    std::vector<trade_event::EventBaseCPtr> process_order(order::OrderBasePtr& o);

    std::vector<std::string> eod_scan() const;

private:
    void initialise(const std::vector<order::OrderBaseCPtr>& orders);

    // need unique ptr here...
    std::vector<trade_event::EventBaseCPtr> cancel_order(int order_id);
    std::vector<trade_event::EventBaseCPtr> insert_order(order::LimitOrderPtr& o);
    std::vector<trade_event::EventBaseCPtr> match_order(order::OrderBasePtr& o);

    typedef std::tuple<stock::stock_symbol, order::order_side> book_key_type;
    std::unordered_map<std::string, order::OrderBookPtr> order_books_;
};

} // namespace exchange

#endif
