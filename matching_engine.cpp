#include <nlohmann/json.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <tuple>
#include <utility>
#include <vector>
#include "matching_engine.hpp"
#include "order.hpp"
#include "price4.hpp"
#include "ticker_rules.hpp"

namespace exchange
{

inline std::string create_book_key(stock::stock_symbol symbol, order::order_side side)
{
    return std::to_string(symbol) + "&" + std::to_string(side);
}

bool MatchingEngine::validate_order(const order::OrderBasePtr& o) const
{
    return o->quantity() > 0;
}

bool MatchingEngine::validate_order(const json& j) const
{
    const stock::stock_symbol symbol = j.value("symbol", stock::stock_symbol::AAPL);
    const int quantity = j.value("quantity", 0);

    // remove invalid symbol
    if (!ticker_rules_->is_valid(symbol))
    {
        return false;
    }
    // remove zero quantity
    if (j.contains("quantity") && quantity == 0)
    {
        return false;
    }

    // if limit order
    if (j.contains("limit_price"))
    {
        const utils::Price4 limit_price = j.value("limit_price", utils::Price4(0));
        if (!ticker_size_rules_->is_valid(limit_price) || 
            !(lot_size_rules_->lot_type(limit_price, quantity) == size_rules::lot_types::round_lot))
        {
            return false;
        }
    }

    return true;
}

bool MatchingEngine::validate_order(const std::string& o) const
{
    auto j = json::parse(o);
    return validate_order(j);
}

std::vector<trade_event::EventBaseCPtr> MatchingEngine::insert_order(order::LimitOrderPtr& o)
{
    const auto& order_book_key = create_book_key(o->symbol(), o->side());
    if (!order_books_.count(order_book_key))
    {
        order_books_[order_book_key] = std::make_unique<order::OrderBook>(
            o->side(), std::vector<order::LimitOrderPtr>()
        );
    }

    const auto msg = order_books_[order_book_key]->insert_order(o);
    return std::vector<trade_event::EventBaseCPtr>(1, msg);
}

MatchingEngine::MatchingEngine(
    const size_rules::TickSizeRulesCPtr& ticker_size_rules,
    const size_rules::LotSizeRulesCPtr& lot_size_rules,
    const ticker_rules::TickerRulesCPtr& ticker_rules
)
:
ticker_size_rules_(ticker_size_rules),
lot_size_rules_(lot_size_rules),
ticker_rules_(ticker_rules)
{}

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
    if (!validate_order(o))
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

void MatchingEngine::eod_cleanup(const std::string& close_order_cache_file)
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

    std::ofstream ofile(close_order_cache_file);
    json j = remaining_orders;
    ofile << j.dump();
}

void MatchingEngine::prev_open_setup(const std::string& close_order_cache_file)
{
    std::ifstream infile(close_order_cache_file);
    if (infile.good())
    {
        std::stringstream buffer;
        buffer << infile.rdbuf();
        json j = json::parse(buffer.str());
        auto orders = j.get<std::vector<std::string>>();

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
}

void update_events(
    std::vector<trade_event::EventBaseCPtr>& events, 
    const std::vector<trade_event::EventBaseCPtr>& insertion_event
)
{
    if (insertion_event.size() != 1)
    {
        throw std::runtime_error("Expect insertion_event has lenght 1.");
    }
    if (events.back()->type() == trade_event::trade_type::depth_update)
    {
        trade_event::DepthUpdateEventCPtr tail_event = std::dynamic_pointer_cast<const trade_event::DepthUpdateEvent>(events.back());
        const trade_event::DepthUpdateEventCPtr new_event = std::dynamic_pointer_cast<const trade_event::DepthUpdateEvent>(insertion_event[0]);
        if ((!tail_event->bid_order_update_info().empty() && !new_event->bid_order_update_info().empty()) ||
            (!tail_event->ask_order_update_info().empty() && !new_event->ask_order_update_info().empty())
        )
        {
            throw std::runtime_error("Expect tail event and insertion event have differnt side.");
        }
        trade_event::EventBaseCPtr merged_event = std::make_shared<trade_event::DepthUpdateEvent>(
            tail_event->bid_order_update_info().empty() ? new_event->bid_order_update_info() : tail_event->bid_order_update_info(),
            tail_event->ask_order_update_info().empty() ? new_event->ask_order_update_info() : tail_event->ask_order_update_info()
        );
        events.pop_back();
        events.push_back(merged_event);
    }
    else
    {
        events.push_back(insertion_event[0]);
    }
}

std::vector<trade_event::EventBaseCPtr> MatchingEngine::process_order(const std::string& s)
{
    std::vector<trade_event::EventBaseCPtr> events;
    try
    {
        json j = json::parse(s);

        if (j.contains("type"))
        {
            if (j.at("type") == "CANCEL")
            {
                cancel_order(j.at("order_id").template get<int>());
            }
            else if (j.at("type") == "NEW")
            {
                order::OrderBasePtr o = order::OrderFactory::create(j);
                events = match_order(o);
                // there is corner case for iceberg order
                if (o->order_type() != order::order_type::market && o->quantity() > 0)
                {
                    order::LimitOrderPtr limit_o = std::dynamic_pointer_cast<order::LimitOrder>(o);
                    auto insertion_event = insert_order(limit_o);
                    update_events(events, insertion_event);
                } 
            }
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }

    return events;
}

} // namespace exchange
