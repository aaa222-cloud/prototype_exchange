#include <nlohmann/json.hpp>
#include "order_book.hpp"
#include "price4.hpp"

namespace order
{

using json = nlohmann::json;

// to do : need better error message and exception handling
void OrderBook::initialise(const std::vector<LimitOrderPtr>& orders)
{
    for (const auto& o : orders)
    {
        if (o->side() != side_)
        {
            throw std::runtime_error("Cannot create order book with order's side different from specified.");
        }
        if (valid_ids_.count(o->order_id()))
        {
            throw std::runtime_error("Order id already exists in order book.");
        }

        order_queue_.push(o);
        valid_ids_.insert(o->order_id());
    }
}

OrderBook::OrderBook(
    order_side side, 
    const std::vector<LimitOrderPtr>& orders
)
:
side_(side)
{
    initialise(orders);
}

trade_event::EventBaseCPtr OrderBook::insert_order(const LimitOrderPtr& o)
{
    if (o->side() != side_)
    {
        throw std::runtime_error("Cannot insert order with mismatch side.");
    }

    if (valid_ids_.count(o->order_id()))
    {
        return std::make_unique<trade_event::DepthUpdateEvent>(
            std::vector<trade_event::OrderUpdateInfoCPtr>(),
            std::vector<trade_event::OrderUpdateInfoCPtr>()
        );
    }
    order_queue_.push(o);
    valid_ids_.insert(o->order_id());

    // need a function here
    std::vector<trade_event::OrderUpdateInfoCPtr> bid_update;
    std::vector<trade_event::OrderUpdateInfoCPtr> ask_update;

    trade_event::OrderUpdateInfoCPtr update = std::make_unique<trade_event::OrderUpdateInfo>(
        o->limit_price(), o->quantity(), trade_event::trade_action::add_add);

    switch (side_)
    {
    case order_side::bid:
        bid_update.push_back(update);
        break;
    
    case order_side::ask:
        ask_update.push_back(update);
        break;

    default:
        break;
    }

    return std::make_unique<trade_event::DepthUpdateEvent>(bid_update, ask_update);
}

trade_event::EventBaseCPtr OrderBook::cancel_order(int order_id)
{
    // well... need optimization...
    if (!valid_ids_.count(order_id))
    {
        return nullptr;
    }
    valid_ids_.erase(order_id);

    std::vector<trade_event::OrderUpdateInfoCPtr> updates;
    std::priority_queue<LimitOrderPtr> backup_queue;
    while (!order_queue_.empty())
    {
        if (order_queue_.top()->order_id() != order_id)
        {
            backup_queue.push(order_queue_.top());
            order_queue_.pop();
            continue;
        }

        order_queue_.pop();
        if (order_queue_.empty()) break;

        const auto& o = order_queue_.top();
        updates.emplace_back(
            std::make_shared<trade_event::OrderUpdateInfo>(
                o->limit_price(), o->quantity(), trade_event::trade_action::add_add)
        );
    }
    std::swap(order_queue_, backup_queue);

    if (side_ == order_side::bid)
    {
        return std::make_unique<trade_event::DepthUpdateEvent>(updates, std::vector<trade_event::OrderUpdateInfoCPtr>());
    }
    return std::make_unique<trade_event::DepthUpdateEvent>(std::vector<trade_event::OrderUpdateInfoCPtr>(), updates);
}

bool OrderBook::order_crossed(const OrderBaseCPtr& o) const
{
    if (o->side() == side_ || order_queue_.empty()) return false;
    if (o->order_type() == order::order_type::market) return true;

    const auto limited_o = std::dynamic_pointer_cast<const order::LimitOrder>(o);
    if (!limited_o)
    {
        throw std::runtime_error("Cannot cast to LimitOrder when checking if orders cross.");
    }

    const auto& o_price = limited_o->limit_price();
    const auto& best_price_in_book = order_queue_.top()->limit_price();
    const bool crossed = o->side() == order_side::bid ? best_price_in_book <= o_price : 
        best_price_in_book >= o_price;

    return crossed;
}

std::vector<trade_event::EventBaseCPtr> OrderBook::match_order(const OrderBasePtr& o)
{
    if (o->side() == side_)
    {
        throw std::runtime_error("Cannot match order with the same side.");
    }

    // agressively reserve memory ?
    std::vector<trade_event::EventBaseCPtr> events; events.reserve(2);
    std::vector<trade_event::OrderUpdateInfoCPtr> updates; updates.reserve(order_queue_.size());
    
    while (o->quantity() > 0 && !order_queue_.empty())
    {
        const auto& target_o = order_queue_.top();
        // if order not valid, skip it
        if (!valid_ids_.count(target_o->order_id()))
        {
            order_queue_.pop();
            continue;
        }
        // if the best order not cross, stop iteration
        if (!order_crossed(o)) break;

        const int full_filled_quantity = std::min(target_o->quantity(), o->quantity());
        o->reduce_quantity(full_filled_quantity);
        target_o->reduce_quantity(full_filled_quantity);
        
        events.emplace_back(
            std::make_unique<trade_event::TradeEvent>(target_o->limit_price(), full_filled_quantity)
        );

        trade_event::trade_action action = trade_event::trade_action::modify;
        if (target_o->quantity() == 0)
        {
            action = trade_event::trade_action::delete_delete;
            order_queue_.pop();
        }
        
        updates.emplace_back(
            std::make_unique<trade_event::OrderUpdateInfo>(target_o->limit_price(), full_filled_quantity, action)
        );
    }

    if (side_ == order_side::bid)
    {
        events.emplace_back(
            std::make_shared<trade_event::DepthUpdateEvent>(updates, std::vector<trade_event::OrderUpdateInfoCPtr>())
        );
    }
    else{
        events.emplace_back(
            std::make_shared<trade_event::DepthUpdateEvent>(std::vector<trade_event::OrderUpdateInfoCPtr>(), updates)
        );
    }
    // note: unfilled limit order will be inserted to other order book - handle outside
    return events;
}

std::vector<std::string> OrderBook::get_eod_orders()
{
    std::vector<std::string> orders; 
    orders.reserve(valid_ids_.size());

    while (!order_queue_.empty())
    {
        const auto& curr_o = order_queue_.top();
        if (curr_o->tif() == order::time_in_force::good_till_cancel && valid_ids_.count(curr_o->order_id()))
        {
            orders.emplace_back(json(curr_o).dump());
        }
        order_queue_.pop();
    }
    return orders;
}

} // namespace order
