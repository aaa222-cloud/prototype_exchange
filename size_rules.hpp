#include <nlohmann/json.hpp>
#include "price4.hpp"

namespace size_rules
{
    enum lot_sizes
    {
        odd_lot,
        round_lot,
        mixed_lot
    };

    // class SingleTickSizeRule
    // {
    // public:
    //     SingleTickSizeRule(
    //         const utils::Price4& from_price, 
    //         const std::optional<utils::Price4>& to_price, 
    //         const utils::Price4& tick_size
    //         )
    //         :
    //         from_price_(from_price),
    //         to_price_(to_price),
    //         tick_size_(tick_size)
    //         {
    //             initialise();
    //         }

    // private:
    //     void initialise();

    //     utils::Price4 from_price_;
    //     std::optional<utils::Price4> to_price_;
    //     utils::Price4 tick_size_;
    // };

    // make SingleSizeRule as template - reuse for both ticker size and lot size
    template <typename T>
    class SingleSizeRule
    {
    public:
        SingleSizeRule(
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

    private:
        void initialise();

        utils::Price4 from_price_;
        std::optional<utils::Price4> to_price_;
        T size_step_;
    };

    using SingleTickSizeRule = SingleSizeRule<utils::Price4>;
    using SingleLotSizeRule = SingleSizeRule<int>;

    // base SizeRules should be template to acommodate different types
    // to do: should we use smart pointer?
    template<typename T>
    class SizeRulesBase
    {
    public:
        SizeRulesBase(const std::vector<T>& size_rules);

        T find_size(const utils::Price4& price) const;

    private:
        void initialise(const std::vector<T>& size_rules);

        std::vector<utils::Price4> critical_prices_;
        std::vector<T> size_steps_;
    };

    class TickSizeRules
    {
    public:
        TickSizeRules(const std::vector<SingleTickSizeRule>& size_rules) 
        {
            initialise(size_rules);
        }

        bool is_valid(const utils::Price4& price) const;

    private:
        void initialise(const std::vector<SingleTickSizeRule>& size_rules);

        std::vector<utils::Price4> critical_prices_;
        std::vector<utils::Price4> tick_sizes_;
    };

    class LotSizeRules
    {
    public:
        LotSizeRules(const std::vector<SingleLotSizeRule>& size_rules)
        {
            initialise(size_rules);
        }

    private:
    };
} // namespace size_rules