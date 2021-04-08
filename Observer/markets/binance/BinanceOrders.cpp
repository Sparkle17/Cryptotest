#include "BinanceOrders.h"

#include "BinanceTrader.h"
#include "..\..\json\json.h"

#include "..\..\Logger.h"

void BinanceOrders::addOrder(const string& orderId)
{
	unique_lock<shared_mutex> lock(m_access);
	m_orders[orderId] = true;
}

void BinanceOrders::connect(shared_ptr<BinanceTrader> trader)
{
	m_trader = trader;
}

void BinanceOrders::disconnect()
{
}

bool BinanceOrders::hasOrder(const string& orderId)
{
	shared_lock<shared_mutex> lock(m_access);
	return m_orders.find(orderId) != m_orders.end();
}

void BinanceOrders::loadOrders()
{
	if (!m_loaded.load()) {
		auto trader = m_trader.lock();
		if (trader) {
			auto api = trader->getApi();
			if (api) {
				bool b = false;
				if (m_loaded.compare_exchange_strong(b, true))
					api->getOrdersData(bind(&BinanceOrders::onOrdersLoaded, shared_from_this(), placeholders::_1, placeholders::_2, placeholders::_3));
				else
					api->release();
			}
		}
	}
}

void BinanceOrders::onOrdersLoaded(const string& data, bool success, HttpResponse response)
{
	if (response.result_int() == 200 && data.size() > 0) {
		try {
			JSONLoader json(data);
			auto obj = json.parse();

			unique_lock<shared_mutex> lock(m_access);
			for (size_t i = 0; i < obj.size(); i++) {
				auto item = move(obj[i]);
				m_orders[item["clientOrderId"].getString()] = true;
			}
			lock.unlock();

			shared_lock<shared_mutex> lockShared(m_access);
			for (auto handler : m_onOrdersLoadedHandlers)
				handler();
		}
		catch (exception) {
			Logger::e("error parsing orders list response");
		}
	}
	else m_loaded = false;
}

void BinanceOrders::registerOnOrdersLoadedHandler(OnOrdersLoaded handler)
{
	unique_lock<shared_mutex> lock(m_access);
	m_onOrdersLoadedHandlers.push_back(handler);
	lock.unlock();

	if (m_loaded.load())
		handler();
}

void BinanceOrders::removeOrder(const string& orderId)
{
	unique_lock<shared_mutex> lock(m_access);
	m_orders.erase(orderId);
}
