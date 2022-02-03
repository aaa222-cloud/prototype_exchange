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
        iceberg,
        unknown
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
            {iceberg, "iceberg"},
            {unknown, "unknown"}
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
    OrderBase() = default;
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

    virtual ~OrderBase() {}

    // should be pure virtual - use this way for serialisation issue
    virtual order::order_type order_type() const { return order::order_type::unknown; };
    virtual int reduce_quantity(int filled_quantity);

    int time() const { return time_; }
    int order_id() const { return order_id_; }
    int quantity() const { return quantity_; }
    order::time_in_force tif() const { return tif_; }

    bool operator==(const OrderBase& a) const;

private:
    // function for serialise
    template <typename BasicJsonType>
    friend void to_json(BasicJsonType& j, const OrderBase& o);
    template <typename BasicJsonType>
    friend void from_json(const BasicJsonType& j, OrderBase& o);

    int time_;
    int order_id_;
    double quantity_;
    order::time_in_force tif_;
};

class LimitOrder : public OrderBase
{
public:
    LimitOrder() = default;
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

    virtual ~LimitOrder() {}

    order::order_type order_type() const override { return order::order_type::limit; }
    const utils::Price4& limit_price() const { return limit_price_; }
    stock::stock_symbol symbol() const { return symbol_; }
    order::order_side side() const { return side_; }

    bool operator==(const LimitOrder& a) const;

private:
    // function for serialise
    template <typename BasicJsonType>
    friend void to_json(BasicJsonType& j, const LimitOrder& o);
    template <typename BasicJsonType>
    friend void from_json(const BasicJsonType& j, LimitOrder& o);

    utils::Price4 limit_price_;
    stock::stock_symbol symbol_;
    order::order_side side_;
};

class MarketOrder : public OrderBase
{
public:
    MarketOrder() = default;
    MarketOrder(
        int time,
        int order_id,
        double quantity,
        order::time_in_force tif,
        stock::stock_symbol symbol,
        order::order_side side
    );

    virtual ~MarketOrder() {}

    order::order_type order_type() const override { return order::order_type::market; }

    stock::stock_symbol symbol() const { return symbol_; }
    order::order_side side() const { return side_; }

    bool operator==(const MarketOrder& a) const;

private:
    // function for serialise
    template <typename BasicJsonType>
    friend void to_json(BasicJsonType& j, const MarketOrder& o);
    template <typename BasicJsonType>
    friend void from_json(const BasicJsonType& j, MarketOrder& o);

    void initialise();

    stock::stock_symbol symbol_;
    order::order_side side_;
};

class IcebergOrder : public OrderBase
{
public:
    IcebergOrder() = default;
    IcebergOrder(
        int time,
        int order_id,
        order::time_in_force tif,
        const utils::Price4& limit_price, 
        stock::stock_symbol symbol,
        order::order_side side,
        double display_quantity,
        double hidden_quantity
    );

    virtual ~IcebergOrder() {}

    order::order_type order_type() const override { return order::order_type::iceberg; }
    int reduce_quantity(int filled_quantity) override;

    const utils::Price4& limit_price() const { return limit_price_; }
    stock::stock_symbol symbol() const { return symbol_; }
    order::order_side side() const { return side_; }
    int display_quantity() const { return display_quantity_; }
    int hidden_quantity() const { return hidden_quantity_; }

    bool operator==(const IcebergOrder& a) const;

private:
    // function for serialise
    template <typename BasicJsonType>
    friend void to_json(BasicJsonType& j, const IcebergOrder& o);
    template <typename BasicJsonType>
    friend void from_json(const BasicJsonType& j, IcebergOrder& o);

    void initialise();

    utils::Price4 limit_price_;
    stock::stock_symbol symbol_;
    order::order_side side_;
    int display_quantity_;
    int hidden_quantity_;
};

// function for serialisation
template <typename BasicJsonType>
void to_json(BasicJsonType& j, const OrderBase& o)
{
    j = BasicJsonType{{"time", o.time_}, {"order_id", o.order_id_}, {"quantity", o.quantity_}, {"tif", o.tif_}};
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, OrderBase& o)
{
    j.at("time").get_to(o.time_);
    j.at("order_id").get_to(o.order_id_);
    j.at("quantity").get_to(o.quantity_);
    j.at("tif").get_to(o.tif_);
}

template <typename BasicJsonType>
void to_json(BasicJsonType& j, const LimitOrder& o)
{
    j = static_cast<OrderBase>(o);
    j["limit_price"] = o.limit_price_;
    j["symbol"] = o.symbol_;
    j["side"] = o.side_;
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, LimitOrder& o)
{   
    nlohmann::from_json(j, static_cast<OrderBase&>(o));
    j.at("limit_price").get_to(o.limit_price_);
    j.at("symbol").get_to(o.symbol_);
    j.at("side").get_to(o.side_);
}

template <typename BasicJsonType>
void to_json(BasicJsonType& j, const MarketOrder& o)
{
    j = static_cast<OrderBase>(o);
    j["symbol"] = o.symbol_;
    j["side"] = o.side_;
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, MarketOrder& o)
{
    nlohmann::from_json(j, static_cast<OrderBase&>(o));
    j.at("symbol").get_to(o.symbol_);
    j.at("side").get_to(o.side_);
}

template <typename BasicJsonType>
void to_json(BasicJsonType& j, const IcebergOrder& o)
{
    j = static_cast<OrderBase>(o);
    j["limit_price"] = o.limit_price_;
    j["symbol"] = o.symbol_;
    j["side"] = o.side_;
    j["display_quantity"] = o.display_quantity_;
    j["hidden_quantity"] = o.hidden_quantity_;
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, IcebergOrder& o)
{
    nlohmann::from_json(j, static_cast<OrderBase&>(o));
    j.at("limit_price").get_to(o.limit_price_);
    j.at("symbol").get_to(o.symbol_);
    j.at("side").get_to(o.side_);
    j.at("display_quantity").get_to(o.display_quantity_);
    j.at("hidden_quantity").get_to(o.hidden_quantity_);
}

} // namespace order

#endif