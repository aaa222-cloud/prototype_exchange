#include <exception>
#include <fstream>
#include <ostream>
#include "exchange.hpp"

using json = nlohmann::json;

void create_rules(
    const std::string config_file, 
    size_rules::TickSizeRulesCPtr& ticker_size_rules,
    size_rules::LotSizeRulesCPtr& lot_size_rules,
    ticker_rules::TickerRulesCPtr& ticker_rules
)
{
    std::ifstream infile(config_file);
    if (infile.good())
    {
        std::stringstream buffer;
        buffer << infile.rdbuf();
        json j = json::parse(buffer.str());

        if (j.contains("lot_size"))
        {
            lot_size_rules = std::make_shared<size_rules::LotSizeRules>(
                j.at("lot_size").get<size_rules::LotSizeRules>());
        }

        if (j.contains("tick_size"))
        {
            ticker_size_rules = std::make_shared<size_rules::TickSizeRules>(
                j.at("tick_size").get<size_rules::TickSizeRules>()
            );
        }

        if (j.constains("symbols"))
        {
            ticker_rules = std::make_shared<ticker_rules::TickerRules>(
                j.at("symbols").get<size_rules::TickSizeRules>()
            );
        }
    }
}

exchange::MarketDataPublisherCPtr create_market_data_publisher(
    const std::string& event_publish_file
)
{
    return std::make_shared<exchange::MarketDataPublisher(event_publish_file);
}

exchange::MatchingEnginePtr create_matching_engine(
    const size_rules::TickSizeRulesCPtr& ticker_size_rules,
    const size_rules::LotSizeRulesCPtr& lot_size_rules,
    const ticker_rules::TickerRulesCPtr& ticker_rules
)
{
    return std::make_shared<exchange::MatchingEngine>(
        std::vector<order::LimitOrderPtr>(), ticker_size_rules, lot_size_rules, ticker_rules
    );
}

namespace exchange
{

void Exchange::initialise(
    const std::string& config_file, 
    const std::string& event_publish_file
)
{
    create_rules(config_file, ticker_size_rules_, lot_size_rules_, ticker_rules_);
    matching_engine_ = create_matching_engine(ticker_size_rules_, lot_size_rules_, ticker_rules_);
    market_data_publisher_ = create_market_data_publisher(event_publish_file);
}

Exchange::Exchange(
    const std::string& config_file, 
    const std::string& event_publish_file,
    const std::string& close_order_cache_file
)
:
close_order_cache_file_(close_order_cache_file)
{
    initialise(config_file, event_publish_file);
}

void Exchange::process_request(const std::string& r)
{
    auto events = matching_engine_->process_order(r);
    //market_data_publisher_ ->publish(events);
    // for test purpose - publish to std::cout
    std::cout = market_data_publisher_ ->publish(std::cout, events);
}

} // namespace exchange