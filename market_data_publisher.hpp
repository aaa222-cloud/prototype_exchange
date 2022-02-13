#ifndef MARKET_DATA_PUBLISHER_
#define MARKET_DATA_PUBLISHER_

#include <ostream>
#include <string>
#include <vector>
#include "event.hpp"

namespace exchange
{

class MarketDataPublisher;
typedef std::unique_ptr<MarketDataPublisher> MarketDataPublisherPtr;
typedef std::unique_ptr<const MarketDataPublisher> MarketDataPublisherCPtr;

class MarketDataPublisher
{
public:
    MarketDataPublisher() = default;
    MarketDataPublisher(
        const std::string& market_data_state_file
    )
    :
    market_data_state_file_(market_data_state_file)
    {}

    void publish(const std:vector<trade_event::EventBaseCPtr>& events) const;
    // write to standard output for test purpose
    std::ostream& publish(std::ostream& os, const std::vector<trade_event::EventBaseCPtr>& events) const;

private:
    std::string market_data_state_file_;
};

} // namespace exchange

#endif
