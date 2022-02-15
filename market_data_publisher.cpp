#include <fstream>
#include "market_data_publisher.hpp"

namespace exchange
{
    std::ostream& MarketDataPublisher::publish(
        std::ostream& os, const std::vector<trade_event::EventBaseCPtr>& events
    ) const
    {
        for (const auto& e : events)
        {
            os << e->to_json() << "\n";
        }
        return os;
    }

void MarketDataPublisher::publish(const std::vector<trade_event::EventBaseCPtr>& events) const
{
    std::ofstream f(market_data_state_file_, std::ios::app);
    for (const auto& e : events)
    {
        f << e->to_json() << "\n";
    }
    f.close();
}

} // namespace exchange
