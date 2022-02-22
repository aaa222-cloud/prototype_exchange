#ifndef ORDER_BOOK_
#define ORDER_BOOK_

#include <memory>
#include <queue>
#include <unordered_set>
#include <vector>

#include "event.hpp"
#include "order.hpp"

namespace order
{

using json = nlohmann::json;

class OrderBookBase;
typedef std::unique_ptr<OrderBookBase> OrderBookPtr;
typedef std::unique_ptr<const OrderBookBase> OrderBookCPtr;

class OrderBookBase
{
public:
    virtual trade_event::EventBaseCPtr insert_order(const LimitOrderPtr& o) = 0;
    virtual trade_event::EventBaseCPtr cancel_order(int order_id) = 0;
    virtual std::vector<trade_event::EventBaseCPtr> match_order(const OrderBasePtr& o) = 0;

    virtual size_t number_of_valid_orders() const = 0;
    virtual const std::unordered_set<int>& valid_ids() const = 0;
    virtual std::vector<std::string> get_eod_orders() = 0;
};

template <typename Comparer>
class OrderBook : public OrderBookBase
{
public:
    OrderBook() = default;
    OrderBook(
        order_side side, 
        const std::vector<LimitOrderPtr>& orders
    );

    trade_event::EventBaseCPtr insert_order(const LimitOrderPtr& o) override;
    trade_event::EventBaseCPtr cancel_order(int order_id) override;
    // one may match LimitOrder, MarketOrder etc. If limit order, there can be unfilled part left
    std::vector<trade_event::EventBaseCPtr> match_order(const OrderBasePtr& o) override;

    size_t number_of_valid_orders() const override { return valid_ids_.size(); }
    const std::unordered_set<int>& valid_ids() const override { return valid_ids_; }
    // not const function because all orders are poped out
    std::vector<std::string> get_eod_orders() override;

private:
    void initialise(const std::vector<LimitOrderPtr>& orders);
    bool order_crossed(const OrderBaseCPtr& o) const;
    void quantity_of_best_price(
        const utils::Price4& trade_price,
        trade_event::trade_action action,
        std::vector<trade_event::OrderUpdateInfoCPtr>& updates
    );
    trade_event::DepthUpdateEventPtr enssemble_depth_update_events(
        const std::vector<trade_event::OrderUpdateInfoCPtr>& updates);

    order_side side_;
    std::priority_queue<LimitOrderPtr, std::vector<LimitOrderPtr>, Comparer> order_queue_;
    std::priority_queue<LimitOrderPtr, std::vector<LimitOrderPtr>, Comparer> hidden_queue_;
    std::unordered_set<int> valid_ids_;
    std::unordered_set<int> hidden_valid_ids_;
};

template <typename Comparer>
void OrderBook<Comparer>::initialise(const std::vector<LimitOrderPtr>& orders)
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

template <typename Comparer>
OrderBook<Comparer>::OrderBook(
    order_side side, 
    const std::vector<LimitOrderPtr>& orders
)
:
side_(side)
{
    initialise(orders);
}

template <typename Comparer>
void OrderBook<Comparer>::quantity_of_best_price(
    const utils::Price4& trade_price,
    trade_event::trade_action action,
    std::vector<trade_event::OrderUpdateInfoCPtr>& updates
)
{
    std::vector<order::LimitOrderPtr> cache; cache.reserve(10);
            
    int open_order_quantity = 0;
    while (!order_queue_.empty() && order_queue_.top()->limit_price() == trade_price)
    {
        open_order_quantity += order_queue_.top()->quantity();
        cache.push_back(order_queue_.top());
        order_queue_.pop();
    }
    while (!cache.empty())
    {
        order_queue_.push(cache.back());
        cache.pop_back();
    }
    updates.emplace_back(
        std::make_shared<trade_event::OrderUpdateInfo>(trade_price, open_order_quantity, action)
    );
}

template <typename Comparer>
trade_event::DepthUpdateEventPtr OrderBook<Comparer>::enssemble_depth_update_events(
    const std::vector<trade_event::OrderUpdateInfoCPtr>& updates
)
{
    if (side_ == order_side::bid)
    {
        return std::make_shared<trade_event::DepthUpdateEvent>(updates, std::vector<trade_event::OrderUpdateInfoCPtr>());
    }
    return std::make_shared<trade_event::DepthUpdateEvent>(std::vector<trade_event::OrderUpdateInfoCPtr>(), updates);
}

template <typename Comparer>
trade_event::EventBaseCPtr OrderBook<Comparer>::insert_order(const LimitOrderPtr& o)
{
    if (o->side() != side_)
    {
        throw std::runtime_error("Cannot insert order with mismatch side.");
    }

    if (valid_ids_.count(o->order_id()))
    {
        return nullptr;
    }
    order_queue_.push(o);
    valid_ids_.insert(o->order_id());

    std::vector<trade_event::OrderUpdateInfoCPtr> updates;
    updates.emplace_back(
        std::make_unique<trade_event::OrderUpdateInfo>(
            o->limit_price(), o->quantity(), trade_event::trade_action::add_add)
    );

    return enssemble_depth_update_events(updates);
}

template <typename Comparer>
trade_event::EventBaseCPtr OrderBook<Comparer>::cancel_order(int order_id)
{
    // optimization?
    if (!valid_ids_.count(order_id))
    {
        return nullptr;
    }
    valid_ids_.erase(order_id);

    utils::Price4 trade_price(0);
    std::vector<trade_event::OrderUpdateInfoCPtr> updates;
    std::vector<LimitOrderPtr> cache; cache.reserve(order_queue_.size());
    while (!order_queue_.empty())
    {
        if (order_queue_.top()->order_id() != order_id)
        {
            cache.push_back(order_queue_.top());
            order_queue_.pop();
            continue;
        }
        order_queue_.pop();
        if (order_queue_.empty()) break;

        auto o = order_queue_.top();
        LimitOrderCPtr limit_o = std::dynamic_pointer_cast<const LimitOrder>(o);
        trade_price = limit_o ? limit_o->limit_price() : trade_price;
        break;
    }
    
    if (trade_price.unscaled() > 0)
    {
        quantity_of_best_price(trade_price, trade_event::trade_action::add_add, updates);
    }

    // put better orders back at the end
    for (const auto& o : cache)
    {
        order_queue_.push(o);
    }

    return enssemble_depth_update_events(updates);
}

template <typename Comparer>
bool OrderBook<Comparer>::order_crossed(const OrderBaseCPtr& o) const
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

template <typename Comparer>
std::vector<trade_event::EventBaseCPtr> OrderBook<Comparer>::match_order(const OrderBasePtr& o)
{
    if (o->side() == side_)
    {
        throw std::runtime_error("Cannot match order with the same side.");
    }

    // agressively reserve memory ?
    std::vector<trade_event::EventBaseCPtr> events; events.reserve(2);
    std::vector<trade_event::OrderUpdateInfoCPtr> updates; updates.reserve(order_queue_.size());
    
    utils::Price4 prev_trade_price(0);
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
        
        const utils::Price4 trade_price = target_o->limit_price();
        const int filled_order_id = target_o->order_id();
        events.emplace_back(
            std::make_shared<trade_event::TradeEvent>(trade_price, full_filled_quantity)
        );

        trade_event::trade_action action = trade_event::trade_action::modify;
        if (target_o->quantity() == 0)
        {
            action = trade_event::trade_action::delete_delete;
            order_queue_.pop();
            valid_ids_.erase(filled_order_id);

            // need to remove redundent updates (i.e. same limit price)
            if (updates.empty() || trade_price != prev_trade_price)
            {
                updates.emplace_back(
                    std::make_shared<trade_event::OrderUpdateInfo>(trade_price, 0, action)
                );
            }
        }
        else
        {
            quantity_of_best_price(trade_price, action, updates);
        }
        prev_trade_price = trade_price;
    }

    if (!updates.empty())
    {
        events.emplace_back(enssemble_depth_update_events(updates));
    }
    
    // note: unfilled limit order will be inserted to other order book - handle outside through matching engine
    return events;
}

template <typename Comparer>
std::vector<std::string> OrderBook<Comparer>::get_eod_orders()
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
    valid_ids_.clear();
    return orders;
}

struct Less
{
    bool operator()(const LimitOrderPtr& a, const LimitOrderPtr& b) const
    {
        return (a->limit_price() < b->limit_price()) || (
            a->limit_price() == b->limit_price() && a->time() > b->time());
    }
};

struct Greater
{
    bool operator()(const LimitOrderPtr& a, const LimitOrderPtr& b) const
    {
        return (a->limit_price() > b->limit_price()) || (
            a->limit_price() == b->limit_price() && a->time() > b->time());
    }
};

typedef OrderBook<Greater> AskOrderBook;
typedef OrderBook<Less> BidOrderBook;

} // namespace order

#endif