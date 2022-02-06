#include "event.hpp"

namespace trade_event
{
void DepthUpdateEvent::add(const OrderUpdateInfoCPtr& new_info, order::order_side side)
{
    switch(side)
    {
    case order::order_side::bid:
        bid_order_update_info_.push_back(new_info);
        break;
    
    case order::order_side::ask:
        ask_order_update_info_.push_back(new_info);
        break;

    default:
        throw std::runtime_error("Unknown order side for depth update event.");
    }
}

} // namespace trade_event
