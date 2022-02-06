# include "order.hpp"

namespace order
{

int OrderBase::reduce_quantity(int filled_quantity)
{
    quantity_ -= filled_quantity;
    return quantity_;
}

bool OrderBase::operator==(const OrderBase& a) const
{
    return (time_ == a.time_ && order_id_ == a.order_id_ && 
    quantity_ == a.quantity_ && tif_ == a.tif_);
}

bool LimitOrder::operator==(const LimitOrder& a) const
{
    return (limit_price_ == a.limit_price_ && symbol_ == a.symbol_ && 
    side_ == a.side_ && OrderBase::operator==(a));
}

bool operator<(const LimitOrder& a, const LimitOrder& b)
{
    if (a.symbol() != b.symbol())
    {
        throw std::runtime_error("Comparison operator only supports limit orders on the same underlier.");
    }

    if (a.side() != b.side())
    {
        throw std::runtime_error("Comparison operator only supports both bid or ask limit orders.");
    }

    // < means lower execution priority
    if (a.side() == order_side::buy)
    {
        return a.limit_price() < b.limit_price() || (a.limit_price() == b.limit_price() && a.time() > b.time());
    }
    return (a.limit_price() > b.limit_price()) || (a.limit_price() == b.limit_price() && a.time() > b.time());
}

bool operator>(const LimitOrder& a, const LimitOrder& b)
{
    return b < a;
}

MarketOrder::MarketOrder(
    int time,
    int order_id,
    double quantity,
    order::time_in_force tif,
    stock::stock_symbol symbol,
    order::order_side side
)
:
OrderBase(time, order_id, quantity, tif),
symbol_(symbol),
side_(side)
{
    initialise();
}

void MarketOrder::initialise()
{
    if (tif() != order::time_in_force::immediate_or_cancel)
    {
        throw std::runtime_error("Time_in_force of market order should be immediate_or_cancel.");
    }
}

bool MarketOrder::operator==(const MarketOrder& a) const
{
    return (symbol_ == a.symbol_ && side_ == a.side_ && OrderBase::operator==(a));
}

void IcebergOrder::initialise()
{
    if (tif() == order::time_in_force::immediate_or_cancel)
    {
        throw std::runtime_error("Time_in_force of iceberg order should be day or good_till_cancel.");
    }
}

IcebergOrder::IcebergOrder(
    int time,
    int order_id,
    order::time_in_force tif,
    const utils::Price4& limit_price, 
    stock::stock_symbol symbol,
    order::order_side side,
    double display_quantity,
    double hidden_quantity
)
:
LimitOrder(time, order_id, 0, tif, limit_price, symbol, side),
display_quantity_(display_quantity),
hidden_quantity_(hidden_quantity)
{
    initialise();
}

int IcebergOrder::reduce_quantity(int filled_quantity)
{
    display_quantity_ -= filled_quantity;
    return display_quantity_;
}

bool IcebergOrder::operator==(const IcebergOrder& a) const
{
    return (limit_price() == a.limit_price() && symbol() == a.symbol() && 
    side() == a.side() && display_quantity_ == a.display_quantity_ && 
    hidden_quantity_ == a.hidden_quantity_ && OrderBase::operator==(a));
}

} // namespace order