#include <iostream>
#include "price4.hpp"
#include "order.hpp"
#include "stock.hpp"

using json = nlohmann::json;

int main(int, char**) {
    // std::cout << "Hello, world!\n";

    // Price4 p1("139.96"), p2("139"), p3("139.01"), p4("139.00");
    // std::cout << p1.unscaled() << ", to_str = " << p1.to_str() << std::endl;
    
    // json j = stock::stock_symbol::AAPL;
    // assert(j == "AAPL");

    // json j2 = "MSFT";
    // assert(j2.get<stock::stock_symbol>() == stock::stock_symbol::MSFT);

    order::LimitOrder limit_order(
        1625787615, 1000134, order::time_in_force::day, utils::Price4("139.96"), 
        stock::stock_symbol::AAPL, order::order_side::buy, 100);
    json j = limit_order;
    std::cout << j << std::endl;

    return 0;
}
