#include "OrderInfo.h"

OrderInfo::OrderInfo(shared_ptr<BinanceTrader> trader, shared_ptr<BookLoader> book, PreciseNumber price, bool buy)
  : m_trader(trader),
	m_book(book),
	m_price(price),
	m_buy(buy),
	m_id(0),
	m_state(OrderState::removed),
	m_locked(false)
{
}

OrderInfo::OrderInfo(shared_ptr<BinanceTrader> trader, shared_ptr<BookLoader> book, PreciseNumber quantity, PreciseNumber price, bool buy)
	: m_trader(trader),
	m_book(book),
	m_price(price),
	m_buy(buy),
	m_id(0),
	m_state(OrderState::removed),
	m_locked(false)
{
	setQuantity(quantity);
}

OrderInfo::OrderInfo(shared_ptr<BinanceTrader> trader, JSONData& data)
	: m_trader(trader),
	m_book(trader->getBook(data["pair"].getString())),
	m_price(PreciseNumber(data["price"].getString())),
	m_buy(data["isbuy"].getBoolean()),
	m_id(data["id"].getInteger()),
	m_state(static_cast<OrderState>(data["state"].getInteger())),
	m_locked(data["locked"].getBoolean())
{
	m_orderQuantity = PreciseNumber(data["quantity"].getString());
	m_sellValue = PreciseNumber(data["sellValue"].getString());
	m_buyValue = PreciseNumber(data["buyValue"].getString());
	m_derivedValue = PreciseNumber(data["derivedValue"].getString());
}

void OrderInfo::calculate()
{
	m_orderQuantity.align(m_book->getMinLotSize());
	m_derivedValue = m_orderQuantity * m_price * m_trader->getComissionMultiplier();
	if (m_buy) {
		m_sellValue = m_orderQuantity * m_price;
		m_buyValue = m_orderQuantity * m_trader->getComissionMultiplier();
	}
	else {
		m_sellValue = m_orderQuantity;
		m_buyValue = m_derivedValue;
	}
}

string OrderInfo::getFullInfo() const
{
	return "id=" + getIdString() + ", " + sellValue().to_string() + " " + sellSymbol() + " -> " + buyValue().to_string() + " " + buySymbol() + " (price=" + m_price.to_string() + ")";
}

Bounds OrderInfo::getInBounds(PreciseNumber percent) const
{
	if (m_buy) {
		auto info = m_book->bestSell();
		return (info.first == 0) ? Bounds::null : (m_price * percent >= info.first ? Bounds::inbound : Bounds::outbound);
	}
	else {
		auto info = m_book->bestBuy();
		return (info.first == 0) ? Bounds::null : (m_price <= info.first * percent ? Bounds::inbound : Bounds::outbound);
	}
}

JSONData OrderInfo::getJSONData() const
{
	auto obj = json::createObject();
	obj["pair"] = json::createString(m_book->getPair());
	obj["quantity"] = json::createString(m_orderQuantity.to_string());
	obj["isbuy"] = json::createBoolean(m_buy);
	obj["price"] = json::createString(m_price.to_string());
	obj["id"] = json::createInteger(m_id);
	obj["state"] = json::createInteger(static_cast<int64_t>(m_state));
	obj["sellValue"] = json::createString(m_sellValue.to_string());
	obj["buyValue"] = json::createString(m_buyValue.to_string());
	obj["derivedValue"] = json::createString(m_derivedValue.to_string());
	obj["locked"] = json::createBoolean(m_locked);
	return obj;
}

PreciseNumber OrderInfo::getProfit(const string& symbol) const
{
	return m_trader->getSymbolCostAs(buySymbol(), buyValue(), symbol) * m_trader->getComissionBase() - m_trader->getSymbolCostAs(sellSymbol(), sellValue(), symbol);
}

bool OrderInfo::isValid() const
{
	return m_derivedValue >= m_book->getNotion();
}

void OrderInfo::setId(int64_t id)
{
	m_id = id;
}

void OrderInfo::setMaxQuantity(PreciseNumber quantity)
{
	m_orderQuantity = min(m_orderQuantity, m_buy ? quantity / m_price : quantity);
	calculate();
}

void OrderInfo::setQuantity(PreciseNumber quantity)
{
	m_orderQuantity = m_buy ? quantity / m_price : quantity;
	calculate();
}
