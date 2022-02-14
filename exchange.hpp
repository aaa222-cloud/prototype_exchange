#ifndef EXCHANGE_H_
#define EXCHANGE_H_

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "market_data_publisher.hpp"
#include "matching_engine.hpp"
#include "size_rules.hpp"
#include "ticker_rules.hppp"


namespace exchange
{

class Exchange
{
public:
    // do I need it?
    Exchange() = default;
    Exchange(
        const std::string& config_file, 
        const std::string& event_publish_file,
        const std::string& close_order_cache_file
    );

    ~Exchange() = default;

    void process_request(const std::string& r);
    void market_open();
    void market_close();

private:
    void initialise(
        const std::string& config_file, 
        const std::string& event_publish_file
    );

    std::string close_order_cache_file_;

    // pointers to size rules
    size_rules::TickSizeRulesCPtr ticker_size_rules_;
    size_rules::LotSizeRulesCPtr lot_size_rules_;
    ticker_rules::TickerRulesCPtr ticker_rules_;

    // pointer to matching engine
    MatchingEnginePtr matching_engine_;
    // pointer to market data publisher
    MarketDataPublisherCPtr market_data_publisher_;
    
};

} // namespace exchange

#endif