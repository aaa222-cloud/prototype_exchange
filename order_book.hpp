#ifndef ORDER_BOOK_
#define ORDER_BOOK_

#include <memory>
#include <nlohmann/json.hpp>
#include <queue>
#include <unordered_set>

#include "event.hpp"
#include "order.hpp"


namespace order
{
class CanceledOrderManager;
class OrderBook;

typedef std::shared_ptr<CanceledOrderManager> CanceledOrderManagerPtr;
typedef std::shared_ptr<const CanceledOrderManager> CanceledOrderManagerCPtr;

typedef std::shared_ptr<OrderBook> OrderBookPtr;
typedef std::shared_ptr<const OrderBook> OrderBookCPtr;


// class used to manage cancelled orders
// class CanceledOrderManager
// {
// public:
//     CanceledOrderManager() = default; // do I need it?
//     CanceledOrderManager(
//         const std::unordered_set<int>& invalid_ids
//     )
//     :
//     invalid_ids_(invalid_ids)
//     {}

//     // no event update for data publisher when canceling order
//     void cancel_order(const OrderBaseCPtr& o);
//     bool is_canceled(const OrderBaseCPtr& o) const;

// private:
//     std::unordered_set<int> invalid_ids_;
// };

class OrderBook
{
public:
    OrderBook() = default;
    OrderBook(
        order_side side, 
        const std::vector<LimitOrderPtr>& orders
    );

    trade_event::EventBaseCPtr insert_order(const LimitOrderPtr& o);
    trade_event::EventBaseCPtr cancel_order(int order_id);
    // one may match LimitOrder, MarketOrder etc. If limit order, there can be unfilled part left
    std::vector<trade_event::EventBaseCPtr> match_order(const OrderBasePtr& o);
    // std::vector<trade_event::EventBaseCPtr> match_order(const LimitOrderPtr& o);

private:
    void initialise(const std::vector<LimitOrderPtr>& orders);
    bool order_crossed(const OrderBaseCPtr& o) const;

    order_side side_;
    std::priority_queue<LimitOrderPtr> order_queue_;
    std::unordered_set<int> valid_ids_;
};

} // namespace order

#endif