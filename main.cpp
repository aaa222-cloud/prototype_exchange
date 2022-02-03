#include <iostream>
#include "price4.hpp"
#include "order.hpp"
#include "stock.hpp"

using json = nlohmann::json;

int main(int, char**) {
    // std::cout << "Hello, world!\n";

    // Price4 p1("139.96"), p2("139"), p3("139.01"), p4("139.00");
    // std::cout << p1.unscaled() << ", to_str = " << p1.to_str() << std::endl;

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
    order::IcebergOrder iceberg_order(
        1625787615, 1000134, order::time_in_force::day, utils::Price4("139.96"),
        stock::stock_symbol::AAPL, order::order_side::buy, 100, 200);
    json j = iceberg_order;
    std::cout << j << std::endl;

    auto iceberg_order2 = j.get<order::IcebergOrder>();
    std::cout << (iceberg_order2 == iceberg_order) << std::endl;

    return 0;
}
