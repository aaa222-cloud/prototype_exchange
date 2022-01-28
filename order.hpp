#ifndef ORDER_H_
#define ORDER_H_

#include <nlohmann/json.hpp>
#include <string>
#include "price4.hpp"
#include "stock.hpp"

// To do: need better serialisation for Order types - especially iceburg order
namespace order
{
    enum order_type
    {
        market,
        limit,
        iceburg
    };

    enum order_side
    {
        buy,
        sell
    };

    enum type
    {
        new_order,
        cancel_order
    };

    enum time_in_force
    {
        day,
        immediate_or_cancel,
        good_till_cancel
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(
        order_type,
        {
            {market, "market"},
            {limit, "limit"},
            {iceburg, "iceburg"}
        }
    )

    NLOHMANN_JSON_SERIALIZE_ENUM(
        order_side,
        {
            {buy, "buy"},
            {sell, "sell"}
        }
    )

    NLOHMANN_JSON_SERIALIZE_ENUM(
        type,
        {
            {new_order, "new"},
            {cancel_order, "cancel"}
        }
    )

    NLOHMANN_JSON_SERIALIZE_ENUM(
        time_in_force,
        {
            {day, "day"},
            {immediate_or_cancel, "immediate_or_cancel"},
            {good_till_cancel, "good_till_cancel"}
        }
    )

class OrderBase
{
public:
    OrderBase(
        int time, 
        int order_id, 
        int quantity,
        order::time_in_force tif
    ) 
    : 
    time_(time), 
    order_id_(order_id),
    quantity_(quantity),
    tif_(tif)
    {}

    virtual order::order_type order_type() const = 0;
    int reduce_quantity(int filled_quantity);

    int time() const { return time_; }
    int order_id() const { return order_id_; }
    int quantity() const { return quantity_; }
    order::time_in_force tif() const { return tif_; }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(OrderBase, time_, order_id_, quantity_, tif_);

private:
    int time_;
    int order_id_;
    double quantity_;
    order::time_in_force tif_;
};

class LimitOrder : public OrderBase
{
public:
    LimitOrder(
        int time,
        int order_id, 
        double quantity,
        order::time_in_force tif,
        const utils::Price4& limit_price,
        stock::stock_symbol symbol,
        order::order_side side
    ) 
    : 
    OrderBase(time, order_id, quantity, tif),
    limit_price_(limit_price),
    symbol_(symbol_),
    side_(side)
    {};

    order::order_type order_type() const override { return order::order_type::limit; }
    const utils::Price4& limit_price() const { return limit_price_; }
    stock::stock_symbol symbol() const { return symbol_; }
    order::order_side side() const { return side_; }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LimitOrder, limit_price_, symbol_, side_);

private:
    utils::Price4 limit_price_;
    stock::stock_symbol symbol_;
    order::order_side side_;
};

class MarketOrder : public OrderBase
{
public:
    MarketOrder(
        int time,
        int order_id,
        double quantity,
        order::time_in_force tif,
        stock::stock_symbol symbol,
        order::order_side side
    )
    :
    OrderBase(time, order_id, quantity, tif),
    symbol_(symbol),
    side_(side)
    {
        initialise();
    }

    order::order_type order_type() const override { return order::order_type::market; }

    stock::stock_symbol symbol() const { return symbol_; }
    order::order_side side() const { return side_; }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MarketOrder, symbol_, side_);

private:
    void initialise();

    stock::stock_symbol symbol_;
    order::order_side side_;
};

class IcebergOrder : public OrderBase
{
public:
    IcebergOrder(
        int time,
        int order_id,
        order::time_in_force tif,
        const utils::Price4& limit_price, 
        stock::stock_symbol symbol,
        order::order_side side,
        double display_quantity,
        double hidden_quantity
    )
    :
    OrderBase(time, order_id, display_quantity, tif),
    limit_price_(limit_price),
    symbol_(symbol),
    side_(side),
    hidden_quantity_(hidden_quantity)
    {
        initialise();
    }

    order::order_type order_type() const override { return order::order_type::iceburg; }

    const utils::Price4& limit_price() const { return limit_price_; }
    stock::stock_symbol symbol() const { return symbol_; }
    order::order_side side() const { return side_; }
    int hidden_quantity() const { return hidden_quantity_; }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(IcebergOrder, limit_price_, symbol_, side_, hidden_quantity_);

private:
    void initialise();

    utils::Price4 limit_price_;
    stock::stock_symbol symbol_;
    order::order_side side_;
    // display_quantity of iceburg order is represented using OrderBase::quantity_
    int hidden_quantity_;
};

} // namespace order

#endif