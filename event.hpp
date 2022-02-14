#ifndef EVENT_H_
#define EVENT_H_

#include <nlohmann/json.hpp>
#include <vector>
#include "order.hpp"
#include "price4.hpp"
#include "serialise.hpp"

namespace trade_event
{

using json = nlohmann::json;

enum trade_action
{
    add_add,
    modify,
    delete_delete
};

enum trade_type
{
    trade,
    depth_update
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    trade_action,
    {
        {add_add, "ADD"},
        {modify, "MODIFY"},
        {delete_delete, "DELETE"}
    }
)

NLOHMANN_JSON_SERIALIZE_ENUM(
    trade_type,
    {
        {trade, "TRADE"},
        {depth_update, "DEPTH_UPDATE"}
    }
)

class EventBase;
class TradeEvent;
class DepthUpdateEvent;
class OrderUpdateInfo;

typedef std::shared_ptr<EventBase> EventBasePtr;
typedef std::shared_ptr<const EventBase> EventBaseCPtr;

typedef std::shared_ptr<TradeEvent> TradeEventPtr;
typedef std::shared_ptr<const TradeEvent> TradeEventCPtr;

typedef std::shared_ptr<OrderUpdateInfo> OrderUpdateInfoPtr;
typedef std::shared_ptr<const OrderUpdateInfo> OrderUpdateInfoCPtr;

typedef std::shared_ptr<DepthUpdateEvent> DepthUpdateEventPtr;
typedef std::shared_ptr<const DepthUpdateEvent> DepthUpdateEventCPtr;

// to do: think about the inheritance structure - kinda wierd...
// need virtual to_json() function
class EventBase
{
public:
    EventBase() = default;
    EventBase(trade_event::trade_type type) : type_(type) {};

    // type() is non-virtual for serialisation purpose
    trade_event::trade_type type() const { return type_; }
    virtual json to_json() const 
    { 
        return json(*this);
    }

private:
    // function for serialise
    template <typename BasicJsonType>
    friend void to_json(BasicJsonType& j, const EventBase& o);
    template <typename BasicJsonType>
    friend void from_json(const BasicJsonType& j, EventBase& o);

    trade_event::trade_type type_;
};

class TradeEvent : public EventBase
{
public:
    TradeEvent() = default;
    TradeEvent(
        const utils::Price4& price,
        int quantity
    )
    :
    EventBase(trade_type::trade),
    price_(price),
    quantity_(quantity)
    {}

    utils::Price4 price() const { return price_; }
    int quantity() const { return quantity_; }

    virtual json to_json() const override
    { 
        return json(*this);
    }

private:
    // function for serialise
    template <typename BasicJsonType>
    friend void to_json(BasicJsonType& j, const TradeEvent& o);
    template <typename BasicJsonType>
    friend void from_json(const BasicJsonType& j, TradeEvent& o);

    utils::Price4 price_;
    int quantity_;
};

// to do: move this class to better place?
class OrderUpdateInfo
{
public:
    OrderUpdateInfo() = default;
    OrderUpdateInfo(
        const utils::Price4& price,
        int quantity,
        trade_event::trade_action action
    )
    :
    price_(price),
    quantity_(quantity),
    action_(action)
    {}

    utils::Price4 price() const { return price_; }
    int quantity() const { return quantity_; }
    trade_event::trade_action action() const { return action_; }

private:
    // function for serialise
    template <typename BasicJsonType>
    friend void to_json(BasicJsonType& j, const OrderUpdateInfo& o);
    template <typename BasicJsonType>
    friend void from_json(const BasicJsonType& j, OrderUpdateInfo& o);

    utils::Price4 price_;
    int quantity_;
    trade_event::trade_action action_;
};

class DepthUpdateEvent : public EventBase
{
public:
    DepthUpdateEvent() = default;
    DepthUpdateEvent(
        const std::vector<OrderUpdateInfoCPtr>& bid_order_update_info,
        const std::vector<OrderUpdateInfoCPtr>& ask_order_update_info
    )
    :
    EventBase(trade_type::depth_update),
    bid_order_update_info_(bid_order_update_info),
    ask_order_update_info_(ask_order_update_info)
    {}

    const std::vector<OrderUpdateInfoCPtr>& bid_order_update_info() const 
    {
        return bid_order_update_info_;
    }

    const std::vector<OrderUpdateInfoCPtr>& ask_order_update_info() const
    {
        return ask_order_update_info_;
    }

    void add(const OrderUpdateInfoCPtr& new_info, order::order_side side);

    virtual json to_json() const override
    { 
        return json(*this);
    }

private:
    // function for serialise
    template <typename BasicJsonType>
    friend void to_json(BasicJsonType& j, const DepthUpdateEvent& o);
    template <typename BasicJsonType>
    friend void from_json(const BasicJsonType& j, DepthUpdateEvent& o);

    std::vector<OrderUpdateInfoCPtr> bid_order_update_info_;
    std::vector<OrderUpdateInfoCPtr> ask_order_update_info_;
};

// function for serialise
template <typename BasicJsonType>
void to_json(BasicJsonType& j, const EventBase& o)
{
    j = BasicJsonType{{"type", o.type_}};
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, EventBase& o)
{
    j.at("type").get_to(o.type_);
}

template <typename BasicJsonType>
void to_json(BasicJsonType& j, const TradeEvent& o)
{
    j = static_cast<EventBase>(o);
    j["price"] = o.price_;
    j["quantity"] = o.quantity_;
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, TradeEvent& o)
{
    nlohmann::from_json(j, static_cast<EventBase&>(o));
    j.at("price").get_to(o.price_);
    j.at("quantity").get_to(o.quantity_);
}

template <typename BasicJsonType>
void to_json(BasicJsonType& j, const OrderUpdateInfo& o)
{
    j = BasicJsonType{{"price", o.price_}, {"quantity", o.quantity_}, {"action", o.action_}};
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, OrderUpdateInfo& o)
{
    j.at("price").get_to(o.price_);
    j.at("quantity").get_to(o.quantity_);
    j.at("action").get_to(o.action_);
}

template <typename BasicJsonType>
void to_json(BasicJsonType& j, const DepthUpdateEvent& o)
{
    j = static_cast<EventBase>(o);
    j["bid"] = o.bid_order_update_info_;
    j["ask"] = o.ask_order_update_info_;
}

template <typename BasicJsonType>
void from_json(const BasicJsonType& j, DepthUpdateEvent& o)
{
    nlohmann::from_json(j, static_cast<EventBase&>(o));
    j.at("bid").get_to(o.bid_order_update_info_);
    j.at("ask").get_to(o.ask_order_update_info_);
}

} // namespace trade_event

#endif
