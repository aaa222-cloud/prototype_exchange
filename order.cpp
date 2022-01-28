# include "order.hpp"

namespace order
{

int OrderBase::reduce_quantity(int filled_quantity)
{
    quantity_ -= filled_quantity;
}

} // namespace order