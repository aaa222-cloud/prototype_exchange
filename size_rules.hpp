#ifndef SIZE_RULES_
#define SIZE_RULES_

#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include "price4.hpp"
#include "serialise.hpp"

namespace size_rules
{
enum lot_types
{
    odd_lot,
    round_lot,
    mixed_lot
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    lot_types,
    {
        {odd_lot, "odd_lot"},
        {round_lot, "round_lot"},
        {mixed_lot, "mixed_lot"}
    }
)

// declare first for friend declarition
template <typename T> class SingleSizeRule;
using SingleTickSizeRule = SingleSizeRule<utils::Price4>;
using SingleLotSizeRule = SingleSizeRule<int>;  

class TickSizeRules;
class LotSizeRules;

typedef std::shared_ptr<TickSizeRules> TickSizeRulesPtr;
typedef std::shared_ptr<const TickSizeRules> TickSizeRulesCPtr;

typedef std::shared_ptr<LotSizeRules> LotSizeRulesPtr;
typedef std::shared_ptr<const LotSizeRules> LotSizeRulesCPtr;

// make SingleSizeRule as template - reuse for both ticker size and lot size
template <typename T>
class SingleSizeRule
{
public:
    typedef T size_step_type;

    SingleSizeRule() = default;
    SingleSizeRule(
        const utils::Price4& from_price,
        const std::optional<utils::Price4>& to_price,
        const T& size_step
    );

    utils::Price4 from_price() const { return from_price_; }
    std::optional<utils::Price4> to_price() const { return to_price_; }
    T size_step() const { return size_step_; }

private:
    // function for serialise - is there a better way?
    template <typename BasicJsonType>
    friend void to_json(BasicJsonType& j, const SingleTickSizeRule& o);
    template <typename BasicJsonType>
    friend void from_json(const BasicJsonType& j, SingleTickSizeRule& o);

    template <typename BasicJsonType>
    friend void to_json(BasicJsonType& j, const SingleLotSizeRule& o);
    template <typename BasicJsonType>
    friend void from_json(const BasicJsonType& j, SingleLotSizeRule& o);

    void initialise();

    utils::Price4 from_price_;
    std::optional<utils::Price4> to_price_;
    T size_step_;
};

template <typename T>
SingleSizeRule<T>::SingleSizeRule(
    const utils::Price4& from_price,
    const std::optional<utils::Price4>& to_price,
    const T& size_step
)
:
from_price_(from_price),
to_price_(to_price),
size_step_(size_step)
{
    initialise();
}

template <typename T>
void SingleSizeRule<T>::initialise()
{
    if (to_price_ && from_price_ > to_price_)
    {
        throw std::runtime_error("From price cannot be less than to price.");
    }
}

// function for serialise
template <typename BasicJsonType>
void to_json(BasicJsonType& j, const SingleTickSizeRule& o)
{
    j = BasicJsonType{{"from_price", o.from_price_}, {"tick_size", o.size_step_}};
    if (o.to_price_)
    {
        j["to_price"] = o.to_price_;
    }
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, SingleTickSizeRule& o)
{
    j.at("from_price").get_to(o.from_price_);
    j.at("tick_size").get_to(o.size_step_);
    if (j.contains("to_price"))
    {
        j.at("to_price").get_to(o.to_price_);
    }
}

template <typename BasicJsonType>
void to_json(BasicJsonType& j, const SingleLotSizeRule& o)
{
    j = BasicJsonType{{"from_price", o.from_price_}, {"lot_size", std::to_string(o.size_step_)}};
    if (o.to_price_)
    {
        j["to_price"] = o.to_price_;
    }
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, SingleLotSizeRule& o)
{
    j.at("from_price").get_to(o.from_price_);
    o.size_step_ = std::stoi(j.at("lot_size").template get<std::string>());
    if (j.contains("to_price"))
    {
        j.at("to_price").get_to(o.to_price_);
    }
}

// base SizeRules is a template to acommodate different size_step types
template <typename T> class SizeRulesBase;
class TickSizeRules;
class LotSizeRules;

template<typename T>
class SizeRulesBase
{
public:
    SizeRulesBase() = default;
    SizeRulesBase(const std::vector<T>& size_rules);

    typename T::size_step_type find_size(const utils::Price4& price) const;
    bool has_rules() const { return !size_rules_.empty(); }

private:
    // function for serialise
    template <typename BasicJsonType>
    friend void to_json(BasicJsonType& j, const TickSizeRules& o);
    template <typename BasicJsonType>
    friend void from_json(const BasicJsonType& j, TickSizeRules& o);

    template <typename BasicJsonType>
    friend void to_json(BasicJsonType& j, const LotSizeRules& o);
    template <typename BasicJsonType>
    friend void from_json(const BasicJsonType& j, LotSizeRules& o);

    void initialise(const std::vector<T>& size_rules);

    // kinda waste of memory, save it for serialisation purpose
    std::vector<T> size_rules_;
    std::vector<utils::Price4> critical_prices_;
    std::vector<typename T::size_step_type> size_steps_;
};

template <typename T>
SizeRulesBase<T>::SizeRulesBase(
    const std::vector<T>& size_rules
)
:
size_rules_(size_rules)
{
    initialise(size_rules);
}

template <typename T>
void SizeRulesBase<T>::initialise(const std::vector<T>& size_rules)
{
    const size_t num_rules = size_rules.size();
    const size_t num_critical_prices = size_rules.back().to_price() ? num_rules + 1 : num_rules;
    
    critical_prices_.resize(num_critical_prices);
    size_steps_.resize(num_rules);

    critical_prices_[0] = size_rules[0].from_price();
    size_steps_[0] = size_rules[0].size_step();
    for (size_t i = 1; i < num_rules; ++i)
    {
        const auto& prev_rule = size_rules[i-1];
        const auto& curr_rule = size_rules[i];

        if (curr_rule.from_price() != prev_rule.to_price())
        {
            throw std::runtime_error("Size rules should specify continuous price intervals.");
        }

        critical_prices_[i] = curr_rule.from_price();
        size_steps_[i] = curr_rule.size_step();
    }

    if (size_rules.back().to_price())
    {
        critical_prices_[num_rules] = *(size_rules.back().to_price());
    }
}

template <typename T>
typename T::size_step_type SizeRulesBase<T>::find_size(const utils::Price4& price) const
{
    const size_t num_prices = critical_prices_.size();
    const size_t num_steps = size_steps_.size();

    const size_t idx = (std::lower_bound(critical_prices_.begin(), critical_prices_.end(), price) 
        - critical_prices_.begin());

    // need to test corner cases
    if ((num_prices > num_steps && idx > num_prices) || idx == 0)
    {
        throw std::runtime_error("No rule specified for the price.");
    }

    return size_steps_[idx-1];
}

class TickSizeRules : public SizeRulesBase<SingleTickSizeRule>
{
public:
    TickSizeRules() = default;
    TickSizeRules(const std::vector<SingleTickSizeRule>& size_rules);

    bool is_valid(const utils::Price4& price) const;
};

class LotSizeRules : public SizeRulesBase<SingleLotSizeRule>
{
public:
    LotSizeRules() = default;
    LotSizeRules(const std::vector<SingleLotSizeRule>& size_rules);

    size_rules::lot_types lot_type(const utils::Price4& price, int lot) const;
};

template <typename BasicJsonType>
void to_json(BasicJsonType& j, const TickSizeRules& o)
{
    j = BasicJsonType(o.size_rules_);
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, TickSizeRules& o)
{
    // to do: implement move constructor
    o = TickSizeRules(j.template get<std::vector<SingleTickSizeRule>>());
}

template <typename BasicJsonType>
void to_json(BasicJsonType& j, const LotSizeRules& o)
{
    j = BasicJsonType{{o.size_rules_}};
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, LotSizeRules& o)
{
    o = LotSizeRules(j.template get<std::vector<SingleLotSizeRule>>());
}

} // namespace size_rules

#endif
