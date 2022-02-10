#include <iostream>
#include "event.hpp"
#include "price4.hpp"
#include "order.hpp"
#include "stock.hpp"
#include "size_rules.hpp"

using json = nlohmann::json;

int main(int, char**) {
    // utils::Price4 p1("139.96"), p2("139"), p3("139.01"), p4("139.");
    // std::cout << p1.unscaled() << ", to_str = " << p1.to_str() << std::endl;
    // std::cout << p2.unscaled() << ", to_str = " << p2.to_str() << std::endl;
    // std::cout << p3.unscaled() << ", to_str = " << p3.to_str() << std::endl;
    // std::cout << p4.unscaled() << ", to_str = " << p4.to_str() << std::endl;

    // test price4 serialisation
    // utils::Price4 p1("139.96");
    // json j = p1;
    // std::cout << j << std::endl;
    // auto p2 = j.get<utils::Price4>();
    // std::cout << "p1 == p2 ? " << (p1 == p2) << std::endl;

    // json j = stock::stock_symbol::AAPL;
    // assert(j == "AAPL");

    // json j2 = "MSFT";
    // assert(j2.get<stock::stock_symbol>() == stock::stock_symbol::MSFT);

    // test limit_order serialisation
    // order::LimitOrder limit_order(
    //     1625787615, 1000134, 100, order::time_in_force::day, utils::Price4("139.96"), 
    //     stock::stock_symbol::AAPL, order::order_side::buy);
    // json j = limit_order;
    // std::cout << j << std::endl;

    // auto limit_order2 = j.get<order::LimitOrder>();
    // assert(limit_order == limit_order2);

    // test market_order serialisation
    // order::MarketOrder market_order(
    //     1625787615, 1000134, 100, order::time_in_force::immediate_or_cancel, stock::stock_symbol::AAPL, 
    //     order::order_side::buy);

    // json j = market_order;
    // std::cout << j << std::endl;

    // auto market_order2 = j.get<order::MarketOrder>();

    // test iceburg_order serialisation
    // order::IcebergOrder iceberg_order(
    //     1625787615, 1000134, order::time_in_force::day, utils::Price4("139.96"),
    //     stock::stock_symbol::AAPL, order::order_side::buy, 100, 200);
    // json j = iceberg_order;
    // std::cout << j << std::endl;

    // auto iceberg_order2 = j.get<order::IcebergOrder>();
    // std::cout << (iceberg_order2 == iceberg_order) << std::endl;

    // test SingleSizeRule
    // size_rules::SingleTickSizeRule rule1(utils::Price4("0"), utils::Price4("1"), utils::Price4("0.0001"));
    // json j1 = rule1;
    // std::cout << j1 << std::endl;
    // auto rule11 = j1.get<size_rules::SingleTickSizeRule>();

    // size_rules::SingleTickSizeRule rule2(utils::Price4("1"), std::nullopt, utils::Price4("0.01"));
    // json j2 = rule2;
    // json j2 =  R"({"from_price": "1", "tick_size": "0.01"})"_json;
    // std::cout << j2 << std::endl;
    // auto rule21 = j2.get<size_rules::SingleTickSizeRule>();
    // json j3 = rule21;
    // std::cout << j3 << std::endl;

    // size_rules::SingleLotSizeRule rule2(utils::Price4("1"), std::nullopt, 100);
    // json j2 = rule2;
    // // json j2 =  R"({"from_price": "1", "lot_size": "100"})"_json;
    // std::cout << j2 << std::endl;
    // auto rule21 = j2.get<size_rules::SingleLotSizeRule>();
    // json j3 = rule21;
    // std::cout << j3 << std::endl;

    // const std::vector<size_rules::SingleTickSizeRule> rules = {
    //     size_rules::SingleTickSizeRule(utils::Price4("0"), utils::Price4("1"), utils::Price4("0.0001")),
    //     size_rules::SingleTickSizeRule(utils::Price4("1"), std::nullopt, utils::Price4("0.01"))
    // };
    // const size_rules::TickSizeRules tick_rules(rules);
    // json j = tick_rules;
    // std::cout << j << std::endl;
    // const auto rules2 = j.get<size_rules::TickSizeRules>();
    // utils::Price4 p1("1.234"), p2("0.234");
    // std::cout << p1.to_str() << " is valid ? " << tick_rules.is_valid(p1) << std::endl;
    // std::cout << p2.to_str() << " is valid ? " << tick_rules.is_valid(p2) << std::endl;

    // test event
    const trade_event::TradeEvent event1(utils::Price4("10.02"), 100);
    json j1 = event1;
    std::cout << j1 << std::endl;
    const auto event12 = j1.get<trade_event::TradeEvent>();

    const std::vector<trade_event::OrderUpdateInfoCPtr> bid_info = {
        std::make_unique<const trade_event::OrderUpdateInfo>(
            utils::Price4("10.01"), 0, trade_event::trade_action::delete_delete
        ),
        std::make_unique<const trade_event::OrderUpdateInfo>(
            utils::Price4("10.00"), 0, trade_event::trade_action::delete_delete
        )
    };
    const std::vector<trade_event::OrderUpdateInfoCPtr> ask_info = {
         std::make_unique<const trade_event::OrderUpdateInfo>(
            utils::Price4("10.00"), 200, trade_event::trade_action::add_add
        )
    };
    const trade_event::DepthUpdateEvent event2(bid_info, ask_info);
    json j2 = event2;
    std::cout << j2 << std::endl;
    const auto event22 = j2.get<trade_event::DepthUpdateEvent>();

    const trade_event::EventBaseCPtr p_event = std::make_unique<trade_event::DepthUpdateEvent>(event2);
    //json j_p = p_event;
    std::cout << "pointer to_json: " << p_event->to_json() << std::endl;

    // test order book


    return 0;
}
