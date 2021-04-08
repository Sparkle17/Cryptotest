#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

#include "BinanceApi.h"
#include "..\..\Network.h"

using namespace std;

class BinanceTrader;

class BinanceOrders: public enable_shared_from_this<BinanceOrders>
{
	typedef function<void()> OnOrdersLoaded;

private:
	weak_ptr<BinanceTrader> m_trader;
	map<string, bool> m_orders;
	atomic<bool> m_loaded;
	mutable shared_mutex m_access;
	vector<OnOrdersLoaded> m_onOrdersLoadedHandlers;

	void onOrdersLoaded(const string& data, bool success, HttpResponse response);

public:
	void addOrder(const string& orderId);
	void removeOrder(const string& orderId);
	bool hasOrder(const string& orderId);

	void connect(shared_ptr<BinanceTrader> trader);
	void disconnect();

	void loadOrders();

	void registerOnOrdersLoadedHandler(OnOrdersLoaded handler);
};
