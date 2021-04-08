#pragma once
#include <memory>

#include "binance/BinanceTrader.h"
#include "BookLoader.h"
#include "../json/json.h"

enum class OrderState { idle = 0, sent = 1, created = 2, completed = 3, removed = 4 };
enum class Bounds { null, inbound, outbound };

class OrderInfo
{
private:
	shared_ptr<BinanceTrader> m_trader;
	shared_ptr<BookLoader> m_book;
	
	PreciseNumber m_orderQuantity;	// значение объёма, передаваемое в создание ордера
	PreciseNumber m_price;
	PreciseNumber m_derivedValue;
	PreciseNumber m_buyValue;
	PreciseNumber m_sellValue;
	bool m_buy;
	int64_t m_id;
	OrderState m_state;
	bool m_locked;

	void calculate();

public:
	OrderInfo(shared_ptr<BinanceTrader> trader, shared_ptr<BookLoader> book, PreciseNumber price, bool buy);
	OrderInfo(shared_ptr<BinanceTrader> trader, shared_ptr<BookLoader> book, PreciseNumber quantity, PreciseNumber price, bool buy);
	OrderInfo(shared_ptr<BinanceTrader> trader, JSONData& data);

	int64_t getId() const
	{
		return m_id;
	}
	string getIdString() const
	{
		return to_string(m_id);
	}
	Bounds getInBounds(PreciseNumber percent) const;
	string getFullInfo() const;
	JSONData getJSONData() const;
	OrderData getOrderData() const
	{
		return { m_book->getPair(), m_buy, m_orderQuantity, m_price, getIdString() };
	}
	string getPair() const
	{
		return m_book->getPair();
	}

	PreciseNumber getProfit(const string& symbol) const;
	OrderState getState() const
	{
		return m_state;
	}

	bool isBuy() const
	{
		return m_buy;
	}
	bool isCompleted() const
	{
		return m_state == OrderState::completed;
	}
	bool isIdle() const
	{
		return m_state == OrderState::idle;
	}
	bool isLocked() const
	{
		return m_locked;
	}
	bool isValid() const;
	void lock()
	{
		m_locked = true;
	}

	string buySymbol() const
	{
		return m_buy ? m_book->getBaseCoin() : m_book->getTradeCoin();
	}
	PreciseNumber buyValue() const
	{
		return m_buyValue;
	}
	string sellSymbol() const
	{
		return m_buy ? m_book->getTradeCoin() : m_book->getBaseCoin();
	}
	PreciseNumber sellValue() const
	{
		return m_sellValue;
	}

	void setId(int64_t id);
	void setMaxQuantity(PreciseNumber quantity);
	void setQuantity(PreciseNumber quantity);
	void setState(OrderState state)
	{
		m_state = state;
	}
};

