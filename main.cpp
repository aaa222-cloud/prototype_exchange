#include <iostream>
#include <unordered_map>
#include "event.hpp"
#include "exchange.hpp"
#include "price4.hpp"
#include "order.hpp"
#include "singleton.hpp"
#include "size_rules.hpp"
#include "ticker_rules.hpp"

using json = nlohmann::json;

int main(int, char**) {
    // test config loading
    std::string config_file = "D:/study/cpp/exchange/financial_exchange/configs/config.json";
    std::string event_publish_file = "D:/study/cpp/exchange/financial_exchange/output/events.json";
    std::string close_order_cache_file = "D:/study/cpp/exchange/financial_exchange/output/eod_orders.json";

    exchange::Exchange e(config_file, event_publish_file, close_order_cache_file);
    // e.market_open();
    // simple test for iceberg order
    // e.process_request(
    //     "{\"time\": 1625787615, \"type\": \"NEW\", \"order_id\": 0, \"symbol\": \"AAPL\", "
    //     "\"side\": \"BUY\", \"display_quantity\": 100, \"hidden_quantity\": 200, "
    //     "\"limit_price\": \"10.01\", \"tif\": \"good_till_cancel\"}"
    //     );
    e.process_request(
        "{\"time\": 1625787615, \"type\": \"NEW\", \"order_id\": 1, \"symbol\": \"AAPL\", "
        "\"side\": \"BUY\", \"quantity\": 100, \"limit_price\": \"10.01\", \"tif\": \"good_till_cancel\"}"
        );
    e.process_request(
        "{\"time\": 1625787616, \"type\": \"NEW\", \"order_id\": 10, \"symbol\": \"AAPL\", "
        "\"side\": \"BUY\", \"quantity\": 200, \"limit_price\": \"10.01\", \"tif\": \"good_till_cancel\"}"
        );
    e.process_request(
        "{\"time\": 1625787615, \"type\": \"NEW\", \"order_id\": 5, \"symbol\": \"AAPL\", "
        "\"side\": \"BUY\", \"quantity\": 500, \"limit_price\": \"10.00\", \"tif\": \"good_till_cancel\"}"
        );
    e.process_request(
        "{\"time\": 1625787615, \"type\": \"NEW\", \"order_id\": 9, \"symbol\": \"AAPL\", "
        "\"side\": \"BUY\", \"quantity\": 500, \"limit_price\": \"9.99\", \"tif\": \"good_till_cancel\"}"
        );
    e.process_request(
        "{\"time\": 1625787615, \"type\": \"NEW\", \"order_id\": 8, \"symbol\": \"AAPL\", "
        "\"side\": \"BUY\", \"quantity\": 500, \"limit_price\": \"9.90\", \"tif\": \"good_till_cancel\"}"
        );

    e.process_request(
        "{\"time\": 1625787615, \"type\": \"NEW\", \"order_id\": 12, \"symbol\": \"AAPL\", "
        "\"side\": \"sell\", \"quantity\": 100, \"limit_price\": \"10.02\", \"tif\": \"good_till_cancel\"}"
        );
    e.process_request(
        "{\"time\": 1625787615, \"type\": \"NEW\", \"order_id\": 2, \"symbol\": \"AAPL\", "
        "\"side\": \"sell\", \"quantity\": 500, \"limit_price\": \"10.03\", \"tif\": \"good_till_cancel\"}"
        );
    e.process_request(
        "{\"time\": 1625787616, \"type\": \"NEW\", \"order_id\": 3, \"symbol\": \"AAPL\", "
        "\"side\": \"sell\", \"quantity\": 200, \"limit_price\": \"10.03\", \"tif\": \"good_till_cancel\"}"
        );
    e.process_request(
        "{\"time\": 1625787615, \"type\": \"NEW\", \"order_id\": 14, \"symbol\": \"AAPL\", "
        "\"side\": \"sell\", \"quantity\": 1000, \"limit_price\": \"10.04\", \"tif\": \"good_till_cancel\"}"
        );
    // e.market_close();
    // e.market_open();

    // test matching order
    // comes an iceberg order - fully matched
    // e.process_request(
    //     "{\"time\": 1625787617, \"type\": \"NEW\", \"order_id\": 0, \"symbol\": \"AAPL\", "
    //     "\"side\": \"BUY\", \"display_quantity\": 100, \"hidden_quantity\": 200, "
    //     "\"limit_price\": \"10.03\", \"tif\": \"good_till_cancel\"}"
    //     );
    // comes an iceberg order - partially matched
    e.process_request(
        "{\"time\": 1625787617, \"type\": \"NEW\", \"order_id\": 0, \"symbol\": \"AAPL\", "
        "\"side\": \"BUY\", \"display_quantity\": 100, \"hidden_quantity\": 900, "
        "\"limit_price\": \"10.03\", \"tif\": \"good_till_cancel\"}"
        );
    e.market_close();
    e.market_open();

    // replenish iceberg order
    e.process_request(
        "{\"time\": 1625787619, \"type\": \"REPLENISH\", \"order_id\": 0, \"quantity\": 100}"
    );

    // order #15
    e.process_request(
        "{\"time\": 1625787616, \"type\": \"NEW\", \"order_id\": 15, \"symbol\": \"AAPL\", "
        "\"side\": \"buy\", \"quantity\": 200, \"limit_price\": \"10.03\", \"tif\": \"good_till_cancel\"}"
    );
    // order #16
    e.process_request(
        "{\"time\": 1625787617, \"type\": \"NEW\", \"order_id\": 16, \"symbol\": \"AAPL\", "
        "\"side\": \"sell\", \"quantity\": 600, \"limit_price\": \"10.02\", \"tif\": \"good_till_cancel\"}"
    );
    // order #17
    e.process_request(
        "{\"time\": 1625787618, \"type\": \"NEW\", \"order_id\": 17, \"symbol\": \"AAPL\", "
        "\"side\": \"sell\", \"quantity\": 1000, \"limit_price\": \"10.00\", \"tif\": \"good_till_cancel\"}"
    );
    // cancel order #2
    e.process_request(
        "{\"time\": 1625787619, \"type\": \"CANCEL\", \"order_id\": 2}"
    );

    return 0;
}
