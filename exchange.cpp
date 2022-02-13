#include <fstream>
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
    if (infile.good)
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

exchange::MarketDataPublisherCPtr create_market_data_publisher()
{

}

exchange::MatchingEnginePtr create_matching_engine()
{
    
}

namespace exchange
{

void Exchange::initialise(
    const std::string& config_file, 
    const std::string& event_publish_file,
    const std::string& close_order_cache_file
)
{
    create_rules(config_file, ticker_size_rules_, lot_size_rules_, ticker_rules_);

}

} // namespace exchange