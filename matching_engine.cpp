#include <nlohmann/json.hpp>
#include <iostream>
#include <tuple>
#include <utility>
#include <vector>
#include "matching_engine.hpp"

namespace exchange
{

using json = nlohmann::json;

inline std::string create_book_key(stock::stock_symbol symbol, order::order_side side)
{
    return std::to_string(symbol) + "&" + std::to_string(side);
}

inline bool order_valid(order::OrderBasePtr& o)
{
    return o->quantity() > 0;
}

std::vector<trade_event::EventBaseCPtr> MatchingEngine::insert_order(order::LimitOrderPtr& o)
{
    const auto& order_book_key = create_book_key(o->symbol(), o->side());
    if (!order_books_.count(order_book_key))
    {
        order_books_[order_book_key] = std::make_shared<order::OrderBook>(
            o->side(), std::vector<order::LimitOrderPtr>()
        );
    }

    const auto msg = order_books_[order_book_key]->insert_order(o);
    return std::vector<trade_event::EventBaseCPtr>(1, msg);
}

MatchingEngine::MatchingEngine(
    const std::vector<order::LimitOrderPtr>& orders
)
{
    // it is not pointer to const object because the orders are not copied
    for (auto o : orders)
    {
        const bool is_limit_order = (o->order_type() == order::order_type::limit || 
            o->order_type() == order::order_type::iceberg);
        
        if (!is_limit_order) continue;

        insert_order(o);
    }
}

MatchingEngine::MatchingEngine(const std::vector<std::string>& orders)
{
    for (const auto& s : orders)
    {
        const auto j = json::parse(s);
        try
        {
            auto o = std::make_shared<order::LimitOrder>(j.get<order::LimitOrder>());
            insert_order(o);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }   
    }
}

std::vector<trade_event::EventBaseCPtr> MatchingEngine::cancel_order(int order_id)
{
    std::vector<trade_event::EventBaseCPtr> msgs;
    for (auto it = order_books_.begin(); it != order_books_.end(); ++it)
    {
        auto msg = it->second->cancel_order(order_id);
        if (msg)
        {
            msgs.push_back(msg);
        }
    }
    return msgs;
}

std::vector<trade_event::EventBaseCPtr> MatchingEngine::match_order(order::OrderBasePtr& o)
{
    if (!order_valid(o))
    {
        return std::vector<trade_event::EventBaseCPtr>();
    }

    const order::order_side book_size = o->side() == order::order_side::bid ?
        order::order_side::ask : order::order_side::bid;

    std::vector<trade_event::EventBaseCPtr> msgs;
    const std::string book_key = create_book_key(o->symbol(), o->side());
    if (order_books_.count(book_key))
    {
        auto& order_book = order_books_[book_key];
        auto msg = order_book->match_order(o);
        msgs.insert(msgs.begin(), msg.begin(), msg.end());
    }

    // this quantity check does not work for iceberg order
    if (o->quantity() > 0 && o->order_type() != order::order_type::market)
    {
        // think about const here
        auto limit_o = std::dynamic_pointer_cast<order::LimitOrder>(o);
        const auto msg = insert_order(limit_o);
        msgs.insert(msgs.begin(), msg.begin(), msg.end());
    }
    return msgs;
}

std::vector<std::string> MatchingEngine::eod_cleanup()
{
    // count number of orders in 1st round (upper bound as some are day order)
    size_t num_orders = 0;
    for (auto it = order_books_.begin(); it != order_books_.end(); ++it)
    {
        num_orders += it->second->number_of_valid_orders();
    }
    std::vector<std::string> remaining_orders; remaining_orders.reserve(num_orders);
    for (auto it = order_books_.begin(); it != order_books_.end(); ++it)
    {
        const auto& curr_book = it->second;
        const auto eod_orders = curr_book->get_eod_orders();
        remaining_orders.insert(remaining_orders.begin(), eod_orders.begin(), eod_orders.end());
    }

    return remaining_orders;
}

} // namespace exchange
