#include "size_rules.hpp"

namespace size_rules
{

TickSizeRules::TickSizeRules(
    const std::vector<SingleTickSizeRule>& size_rules
)
:
SizeRulesBase<SingleTickSizeRule>(size_rules)
{}

bool TickSizeRules::is_valid(const utils::Price4& price) const
{
    if (!has_rules) { return true; }
    const auto tick_size = find_size(price);
    const bool valid = (price.unscaled() % tick_size.unscaled() == 0);
    return valid;
}

LotSizeRules::LotSizeRules(
    const std::vector<SingleLotSizeRule>& size_rules
)
:
SizeRulesBase<SingleLotSizeRule>(size_rules)
{}

size_rules::lot_types LotSizeRules::lot_type(const utils::Price4& price, int lot) const
{
    if (!has_rules) { return size_rules::mixed_lot; }

    const int step_size = find_size(price);
    if (lot < step_size)
    {
        return size_rules::odd_lot;
    }

    if (lot % step_size == 0)
    {
        return size_rules::round_lot;
    }
    
    return size_rules::mixed_lot;
}

} // namespace size_rules