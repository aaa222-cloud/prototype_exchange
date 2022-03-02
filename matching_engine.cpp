#include <nlohmann/json.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include "matching_engine.hpp"
#include "order.hpp"
#include "price4.hpp"
#include "ticker_rules.hpp"

namespace exchange
{

inline std::string create_book_key(const std::string& symbol, order::order_side side)
{
    return symbol + "&" + std::to_string(side);
}

bool MatchingEngine::validate_order(const order::OrderBasePtr& o) const
{
    return o->quantity() > 0;
}

bool MatchingEngine::validate_order(const json& j) const
{
    const std::string symbol = j.value("symbol", "AAPL");
    const int quantity = j.contains("quantity") ? j.at("quantity").get<int>() : 
        (j.value("display_quantity", 0) + j.value("hidden_quantity", 0));

    // remove invalid symbol
    if (!ticker_rules_->is_valid(symbol))
    {
        return false;
    }
    // remove zero quantity
    if (quantity == 0)
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
        if (o->side() == order::order_side::bid)
        {
            order_books_[order_book_key] = std::make_unique<order::BidOrderBook>(
                o->side(), std::vector<order::LimitOrderPtr>());
        }
        else
        {
            order_books_[order_book_key] = std::make_unique<order::AskOrderBook>(
                o->side(), std::vector<order::LimitOrderPtr>());
        }
    }

    auto msg = order_books_[order_book_key]->insert_order(o);
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
    msgs.reserve(1);
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

std::vector<trade_event::EventBaseCPtr> MatchingEngine::replenish_order(int order_id, int quantity)
{
    std::vector<trade_event::EventBaseCPtr> msgs;
    msgs.reserve(1);
    for (auto it = order_books_.begin(); it != order_books_.end(); ++it)
    {
        const std::string& key = it->first;
        const std::string symbol = key.substr(0, key.find("&"));
        auto msg = it->second->replenish_order(order_id, quantity, symbol);
        if (msg)
        {
            msgs.push_back(msg);
        }
    }
    return msgs;
}

std::vector<trade_event::EventBaseCPtr> MatchingEngine::match_order(order::OrderBasePtr& o)
{
    const order::order_side book_side = o->side() == order::order_side::bid ?
        order::order_side::ask : order::order_side::bid;

    std::vector<trade_event::EventBaseCPtr> msgs;
    const std::string book_key = create_book_key(o->symbol(), book_side);
    if (order_books_.count(book_key))
    {
        auto& order_book = order_books_[book_key];
        auto msg = order_book->match_order(o);
        msgs.insert(msgs.begin(), msg.begin(), msg.end());
    }
    return msgs;
}

void MatchingEngine::eod_cleanup(const std::string& close_order_cache_file)
{
    std::ofstream ofile(close_order_cache_file);
    for (auto it = order_books_.begin(); it != order_books_.end(); ++it)
    {
        const auto& curr_book = it->second;
        const auto eod_orders = curr_book->get_eod_orders();
        for (const auto& o : eod_orders)
        {
            ofile << o << "\n";
        } 
    }
}

std::vector<trade_event::EventBaseCPtr> MatchingEngine::prev_open_setup(
    const std::string& close_order_cache_file
)
{
    std::ifstream infile(close_order_cache_file);
    std::string butter;
    while (std::getline(infile, butter))
    {
        if (!butter.empty())
        {
            const json j = json::parse(butter);
            try
            {
                order::OrderBasePtr base_o = order::OrderFactory::create(j);
                order::LimitOrderPtr o = std::dynamic_pointer_cast<order::LimitOrder>(base_o);
                insert_order(o);
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
        }
    }

    std::vector<trade_event::EventBaseCPtr> info;
    const size_t num_books = order_books_.size();
    info.reserve(num_books);
    for (const auto& [key, book] : order_books_)
    {   
        const auto event = book->get_price_levels(key.substr(0, key.find("&")));
        info.push_back(event);
    }
    return info;
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
    if (!events.empty() && events.back()->type() == trade_event::trade_type::depth_update)
    {
        if (events.back()->empty())
        {
            events.pop_back();
            if (insertion_event[0])
                events.push_back(insertion_event[0]);
        }
        else
        {
            trade_event::DepthUpdateEventCPtr tail_event = std::dynamic_pointer_cast<const trade_event::DepthUpdateEvent>(events.back());
            trade_event::DepthUpdateEventCPtr new_event = std::dynamic_pointer_cast<const trade_event::DepthUpdateEvent>(insertion_event[0]);
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
    }
    else
    {   
        if (insertion_event[0]) events.push_back(insertion_event[0]);
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
                events = cancel_order(j.at("order_id").template get<int>());
            }
            else if (j.at("type") == "REPLENISH")
            {
                events = replenish_order(
                    j.at("order_id").template get<int>(), 
                    j.at("quantity").template get<int>()
                );
            }
            else if (j.at("type") == "NEW")
            {
                if (validate_order(j))
                {
                    order::OrderBasePtr o = order::OrderFactory::create(j);
                    events = match_order(o);
                    // there is corner case for iceberg order
                    if (o->order_type() != order::order_type::market && o->total_quantity() > 0)
                    {
                        order::LimitOrderPtr limit_o = std::dynamic_pointer_cast<order::LimitOrder>(o);
                        auto insertion_event = insert_order(limit_o);
                        update_events(events, insertion_event);
                    }
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
