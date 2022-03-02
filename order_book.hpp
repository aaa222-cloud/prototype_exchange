#ifndef ORDER_BOOK_
#define ORDER_BOOK_

#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "event.hpp"
#include "order.hpp"
#include "price4.hpp"
#include "utils.hpp"

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
    virtual trade_event::EventBaseCPtr replenish_order(
        int order_id, int quantity, const std::string& symbol) = 0;

    virtual size_t number_of_valid_orders() const = 0;
    virtual const std::unordered_set<int>& valid_ids() const = 0;
    virtual std::vector<std::string> get_eod_orders() = 0;
    virtual trade_event::EventBaseCPtr get_price_levels(const std::string& symbol) const = 0;
};

struct OrderInfo
{
    utils::Price4 price;
    int quantity;
    time_in_force tif;
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
    trade_event::EventBaseCPtr replenish_order(
        int order_id, int quantity, const std::string& symbol) override;

    size_t number_of_valid_orders() const override { return valid_ids_.size(); }
    const std::unordered_set<int>& valid_ids() const override { return valid_ids_; }
    // not const function because all orders are poped out
    std::vector<std::string> get_eod_orders() override;
    trade_event::EventBaseCPtr get_price_levels(const std::string& symbol) const override;

private:
    void insert_order(const LimitOrderPtr& o, int);
    std::vector<trade_event::EventBaseCPtr> match_order(
        const utils::Price4& limit_price, 
        int& quantity
    );
    void match_at_given_price(
        const utils::Price4& price_level,
        int& quantity,
        std::priority_queue<LimitOrderPtr, std::vector<LimitOrderPtr>, Comparer>& order_queue,
        std::unordered_set<int>& valid_ids,
        std::unordered_map<int, OrderInfo>& order_info,
        bool public_queue,
        std::vector<trade_event::EventBaseCPtr>& trade_events,
        std::vector<trade_event::OrderUpdateInfoCPtr>& updates
    );

    void initialise(const std::vector<LimitOrderPtr>& orders);
    bool order_crossed(
        const OrderBaseCPtr& o, 
        const std::priority_queue<LimitOrderPtr, std::vector<LimitOrderPtr>, Comparer>& order_queue
    ) const;
    utils::Price4 get_best_price() const;
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
    std::unordered_map<const utils::Price4, int> price_levels_;
    std::unordered_map<int, OrderInfo> order_info_;
    //only used replenish iceberg order
    std::unordered_map<int, OrderInfo> hidden_order_info_;
};

template <typename Comparer>
void OrderBook<Comparer>::insert_order(const LimitOrderPtr& o, int)
{
    const int order_id = o->order_id();
    const auto& limit_price = o->limit_price();
    const int quantity = o->quantity();
    const time_in_force tif = o->tif();

    if (o->order_type() == order::order_type::limit)
    {
        order_queue_.push(o);
        valid_ids_.insert(order_id);
        price_levels_[limit_price] += quantity;
    }
    else
    {
        IcebergOrderPtr iceberg_o = std::dynamic_pointer_cast<IcebergOrder>(o);
        if (iceberg_o)
        {
            const auto splitted_orders = iceberg_o->split_order();
            const auto& displayed_o = splitted_orders[0];

            if (quantity > 0)
            {
                order_queue_.push(displayed_o);
                valid_ids_.insert(order_id);
                price_levels_[limit_price] += quantity;
            }

            hidden_queue_.push(splitted_orders[1]);
            hidden_valid_ids_.insert(order_id);
            hidden_order_info_[order_id] = OrderInfo{limit_price, iceberg_o->hidden_quantity(), tif};
        }
    }
    order_info_[order_id] = OrderInfo{limit_price, quantity, tif};
}

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

        insert_order(o, 1);
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
    const int open_order_quantity = price_levels_[trade_price];
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
    insert_order(o, 1);

    std::vector<trade_event::OrderUpdateInfoCPtr> updates;
    if (o->quantity() > 0)
    {
        updates.emplace_back(
            std::make_unique<trade_event::OrderUpdateInfo>(
                o->limit_price(), o->quantity(), trade_event::trade_action::add_add)
        );
    } 

    return enssemble_depth_update_events(updates);
}

template <typename Comparer>
trade_event::EventBaseCPtr OrderBook<Comparer>::cancel_order(int order_id)
{
    if (hidden_valid_ids_.count(order_id))
    {
        hidden_valid_ids_.erase(order_id);
    }
    
    if (!valid_ids_.count(order_id))
    {
        return nullptr;
    }
    valid_ids_.erase(order_id);

    const utils::Price4 trade_price = order_info_[order_id].price;
    const int quantity = order_info_[order_id].quantity;
    if (!price_levels_.count(trade_price))
    {
        throw std::runtime_error("Inconsistent price levels information.");
    }
    price_levels_[trade_price] -= quantity;

    std::vector<trade_event::OrderUpdateInfoCPtr> updates;
    if (trade_price.unscaled() > 0)
    {
        quantity_of_best_price(trade_price, trade_event::trade_action::add_add, updates);
    }

    return enssemble_depth_update_events(updates);
}

template <typename Comparer>
trade_event::EventBaseCPtr OrderBook<Comparer>::replenish_order(
    int order_id, int quantity, const std::string& symbol
)
{
    if (valid_ids_.count(order_id) && order_info_[order_id].quantity > 0)
    {
        // replenish only works after displayed part full filled
        return nullptr;
    }
    if (!hidden_valid_ids_.count(order_id) || hidden_order_info_[order_id].quantity == 0)
    {
        return nullptr;
    }
    const int exposed_quantity = std::min(quantity, hidden_order_info_[order_id].quantity);
    hidden_order_info_[order_id].quantity -= exposed_quantity;
    
    const OrderInfo& info = hidden_order_info_[order_id];
    LimitOrderPtr o = std::make_shared<LimitOrder>(
        utils::get_epoch_time(), 
        order_id, 
        quantity,
        info.tif,
        info.price,
        symbol,
        side_
    );
    return insert_order(o);
}

template <typename Comparer>
bool OrderBook<Comparer>::order_crossed(
    const OrderBaseCPtr& o,
    const std::priority_queue<LimitOrderPtr, std::vector<LimitOrderPtr>, Comparer>& order_queue
) const
{
    if (o->side() == side_ || order_queue.empty()) return false;
    if (o->order_type() == order::order_type::market) return true;

    const auto limited_o = std::dynamic_pointer_cast<const order::LimitOrder>(o);
    if (!limited_o)
    {
        throw std::runtime_error("Cannot cast to LimitOrder when checking if orders cross.");
    }

    const auto& o_price = limited_o->limit_price();
    const auto& best_price_in_book = order_queue.top()->limit_price();
    const bool crossed = o->side() == order_side::bid ? best_price_in_book <= o_price : 
        best_price_in_book >= o_price;

    return crossed;
}

template <typename Comparer>
utils::Price4 OrderBook<Comparer>::get_best_price() const
{
    if (order_queue_.empty() && hidden_queue_.empty())
    {
        return utils::Price4(0);
    }
    if (order_queue_.empty())
    {
        return hidden_queue_.top()->limit_price();
    }
    if (hidden_queue_.empty())
    {
        return order_queue_.top()->limit_price();
    }
    return std::min(order_queue_.top()->limit_price(), hidden_queue_.top()->limit_price());
}

template <typename Comparer>
void OrderBook<Comparer>::match_at_given_price(
    const utils::Price4& price_level,
    int& quantity,
    std::priority_queue<LimitOrderPtr, std::vector<LimitOrderPtr>, Comparer>& order_queue,
    std::unordered_set<int>& valid_ids,
    std::unordered_map<int, OrderInfo>& order_info,
    bool public_queue,
    std::vector<trade_event::EventBaseCPtr>& trade_events,
    std::vector<trade_event::OrderUpdateInfoCPtr>& updates
)
{
    // see if the logic can be simplified
    if (order_queue.empty() || quantity == 0 || price_level != order_queue.top()->limit_price())
    {
        return;
    }
    
    utils::Price4 prev_trade_price(0);
    while (quantity > 0 && !order_queue.empty())
    {
        const auto& target_o = order_queue.top();
        const int target_o_id = target_o->order_id();
        // if order not valid, skip it
        if (!valid_ids.count(target_o_id))
        {
            order_queue.pop();
            continue;
        }
        // if the current price level is exhausted, stop iteration
        if (target_o->limit_price() != price_level) break;

        const int target_o_qty = order_info[target_o_id].quantity;
        const int full_filled_quantity = std::min(target_o_qty, quantity);
        quantity -= full_filled_quantity;
        order_info[target_o_id].quantity -= full_filled_quantity;
        // quantity on order level is redundant
        target_o->set_quantity(order_info[target_o_id].quantity);

        const utils::Price4 trade_price = target_o->limit_price();
        
        if (public_queue)
        {
            price_levels_[trade_price] -= full_filled_quantity;
            if (price_levels_[trade_price] == 0)
            {
                price_levels_.erase(trade_price);
            }
        }
        
        trade_events.emplace_back(
            std::make_shared<trade_event::TradeEvent>(trade_price, full_filled_quantity)
        );

        if (target_o->quantity() == 0)
        {
            order_queue.pop();
            valid_ids.erase(target_o_id);
            order_info.erase(target_o_id);

            if (public_queue)
            {
                // need to remove redundent updates (i.e. same limit price)
                if (updates.empty() || trade_price != prev_trade_price)
                {
                    updates.emplace_back(
                        std::make_shared<trade_event::OrderUpdateInfo>(
                            trade_price, 0, trade_event::trade_action::delete_delete)
                    );
                }
            }
        }
        else if (public_queue)
        {
            quantity_of_best_price(trade_price, trade_event::trade_action::modify, updates);
        }
        prev_trade_price = trade_price;
    }
}

template <typename Comparer>
std::vector<trade_event::EventBaseCPtr> OrderBook<Comparer>::match_order(const OrderBasePtr& o)
{
    if (o->side() == side_)
    {
        throw std::runtime_error("Cannot match order with the same side.");
    }

    std::vector<trade_event::EventBaseCPtr> trade_events;
    std::vector<trade_event::OrderUpdateInfoCPtr> updates;
    bool order_cross = order_crossed(o, order_queue_) || order_crossed(o, hidden_queue_);
    if (!order_cross) { return trade_events; }

    utils::Price4 curr_price = get_best_price();
    // if o is iceberg order, so use total quantity
    int quantity = o->total_quantity();

    while (order_cross && quantity > 0)
    {
        // public queue
        match_at_given_price(
                curr_price, quantity, order_queue_, valid_ids_, order_info_, true, trade_events, updates
            );

        // hidden queue
        match_at_given_price(
            curr_price, quantity, hidden_queue_, hidden_valid_ids_, hidden_order_info_, false, trade_events, 
            updates
        );

        curr_price = get_best_price();
        order_cross = order_crossed(o, order_queue_) || order_crossed(o, hidden_queue_);
    }
    const int fullfilled_quantity = o->total_quantity() - quantity;
    if (fullfilled_quantity > o->quantity())
    {
        o->set_quantity(0);
        auto iceberg_o = std::dynamic_pointer_cast<IcebergOrder>(o);
        if (!iceberg_o)
        {
            throw std::runtime_error("Filled more than quantity for non-iceberg order.");
        }
        iceberg_o->set_hidden_quantity(quantity);
    }
    else
    {
        o->set_quantity(quantity);
    }

    trade_events.emplace_back(enssemble_depth_update_events(updates));
    // note: unfilled limit order will be inserted to other order book - handle outside through matching engine
    return trade_events;
}

template <typename Comparer>
std::vector<std::string> OrderBook<Comparer>::get_eod_orders()
{
    std::vector<std::string> orders; 
    orders.reserve(valid_ids_.size());

    std::unordered_map<int, LimitOrderPtr> cache;
    while (!order_queue_.empty())
    {
        const auto& curr_o = order_queue_.top();
        const int order_id = curr_o->order_id();
        if (curr_o->tif() == order::time_in_force::good_till_cancel && valid_ids_.count(order_id))
        {
            // cache in order to put iceberg order back
            if (hidden_valid_ids_.count(order_id))
            {
                cache[order_id] = curr_o;
            }
            else
            {
                orders.emplace_back(json(curr_o).dump());
            } 
        }
        order_queue_.pop();
    }
    valid_ids_.clear();

    while (!hidden_queue_.empty())
    {
        const auto& curr_o = hidden_queue_.top();
        const int order_id = curr_o->order_id();
        if (curr_o->tif() == order::time_in_force::good_till_cancel && hidden_valid_ids_.count(order_id))
        {
            const LimitOrderCPtr display_o = cache.count(order_id) ? cache[order_id] : LimitOrderCPtr();
            auto iceberg_o = std::make_shared<order::IcebergOrder>(display_o, curr_o);
            iceberg_o->set_hidden_quantity(hidden_order_info_[order_id].quantity);
            orders.emplace_back(json(iceberg_o).dump());
        }
        hidden_queue_.pop();
    }
    hidden_valid_ids_.clear();
    hidden_order_info_.clear();

    // also clear other cached information
    price_levels_.clear();
    order_info_.clear();

    return orders;
}

namespace
{
struct PriceInfoLessThan
{
    bool operator()(const std::pair<utils::Price4, int>& a, const std::pair<utils::Price4, int>& b)
    {
        return a.first < b.first;
    }
};

struct PriceInfoGreaterThan
{
    bool operator()(const std::pair<utils::Price4, int>& a, const std::pair<utils::Price4, int>& b)
    {
        return a.first > b.first;
    }
};
} // anonymous namespace

template <typename Comparer>
trade_event::EventBaseCPtr OrderBook<Comparer>::get_price_levels(
    const std::string& symbol
) const
{
    std::vector<std::pair<utils::Price4, int>> info; 
    info.reserve(price_levels_.size());
    for (const auto& kv : price_levels_)
    {
        info.emplace_back(std::pair<utils::Price4, int>(kv.first, kv.second));
    }
    if (side_ == order::order_side::bid)
    {
        std::sort(info.begin(), info.end(), PriceInfoGreaterThan());
    }
    else
    {
        std::sort(info.begin(), info.end(), PriceInfoLessThan());
    }
    
    return std::make_shared<trade_event::MarketSnapEvent>(side_, symbol, info);
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