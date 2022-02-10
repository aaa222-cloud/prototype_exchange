#ifndef ORDER_BOOK_
#define ORDER_BOOK_

#include <memory>
#include <queue>
#include <unordered_set>

#include "event.hpp"
#include "order.hpp"

namespace order
{
class OrderBook;
typedef std::unique_ptr<OrderBook> OrderBookPtr;
typedef std::unique_ptr<const OrderBook> OrderBookCPtr;

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

    size_t number_of_valid_orders() const { return valid_ids_.size(); }
    // make sense ?
    const std::priority_queue<LimitOrderPtr>& order_queue() const { return order_queue_; }
    const std::unordered_set<int>& valid_ids() const { return valid_ids_; }
    // not const function because all orders are poped out
    std::vector<std::string> get_eod_orders();

private:
    void initialise(const std::vector<LimitOrderPtr>& orders);
    bool order_crossed(const OrderBaseCPtr& o) const;

    order_side side_;
    std::priority_queue<LimitOrderPtr> order_queue_;
    std::unordered_set<int> valid_ids_;
};

} // namespace order

#endif