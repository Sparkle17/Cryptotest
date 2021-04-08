#pragma once
#include <functional>
#include <memory>
#include <shared_mutex>
#include <vector>

#include "BinanceApi.h"
#include "..\BookLoader.h"
#include "BinanceOrders.h"
#include "BinanceWebsocket.h"
#include "..\..\Network.h"

using namespace std;

class OrderInfo;

struct BalanceRecord
{
	PreciseNumber balanceStart;
	PreciseNumber balance;				// свободный баланс
	PreciseNumber balanceLocked;		// баланс, заблокированный биржей (не входит в balance)
	PreciseNumber balanceInNewOrders;	// баланс, заблокированный при постановке ордера (не входит в баланс)
	PreciseNumber balanceReserved;
};

struct PairInfo
{
	string base;
	string quoted;
	PreciseNumber minLotSize;
	PreciseNumber notion;
};

struct PairTradeInfo {
	shared_ptr<BookLoader> book;
	bool direction;
};

class BookLoaderBinance;

class BinanceTrader: public enable_shared_from_this<BinanceTrader>
{
	friend BinanceOrders;

	typedef function<void(const string&)> OnBalanceChanged;
	typedef function<void()> OnBookListChanged;
	typedef function<void(const string&, const string&, const string&, bool)> OnOrderStatusChanged;

	class UserStreamCreator: public enable_shared_from_this<UserStreamCreator>
	{
	private:
		shared_ptr<BinanceWebsocket> m_userDataStream;
		string m_userStreamKey;
		atomic<bool> m_connected;

	public:
		UserStreamCreator() : m_connected(false) {}

		void connect(shared_ptr<BinanceApi> api, WebsocketRequestHandler handler);
		void disconnect();
		void refresh(shared_ptr<BinanceApi> api);
	};

private:
	static const int maxApi = 10;
	static const string dataPath;

	mutable shared_mutex m_APIAccess;
	string m_APIKey;
	string m_APISecret;
	volatile int64_t m_timestampDelta;
	vector<shared_ptr<BinanceApi>> m_apis;

	int64_t getSystemTimeAsUnixTime();
	void updateServerTime(const string& data);

	map<string, PairInfo> m_info;
	thread m_apiRefresher;

	mutable shared_mutex m_balanceAccess;
	vector<string> m_baseSymbols;
	map<string, BalanceRecord> m_balance;

	shared_ptr<BinanceApi> getApi();
	void onApiConnected(bool success);
	void refreshApi();

	PreciseNumber m_comissionMultiplier;	// умножить на это число для получения значения за вычетом комиссии
	PreciseNumber m_comissionDivider;		// умножить на это число для получения значения, которое было до применения комиссии
	PreciseNumber m_comissionBase;			// комиссия в валюте отличной от валют сделки (задана как множитель)

	atomic<bool> m_connected;
	atomic<bool> m_infoRequested;
	atomic<bool> m_balanceRequested;
	atomic<bool> m_timeRefreshed;

	bool createOrderInternal(const OrderInfo& order, shared_ptr<BinanceApi> api);
	void onTradeInfo(string data, bool success);
	void refreshData();
	void refreshTime();
	void requestBalance();
	void requestTradeInfo();

	mutable shared_mutex m_booksAccess;
	map<string, shared_ptr<BookLoaderBinance>> m_books;
	thread m_bookLoader;

	shared_mutex m_handlersAccess;
	vector<OnBalanceChanged> m_onBalanceChanged;
	vector<OnBookListChanged> m_onBookListChanged;
	vector<OnOrderStatusChanged> m_onOrderStatusChanged;

	void loadBooks();
	void onBalanceChanged(const string& coin);
	void onBalanceInfo(const string& data, bool success, HttpResponse response);
	void onBooksListChanged();
	void onOrderStatusChanged(const string& orderUserId, const string& executionType, const string& currentStatus, bool inBook);

	shared_ptr<UserStreamCreator> m_userStream;
	void userStreamHandler(string data, bool success);

	void loadBalance();
	void saveBalance();
	void setCoinBalance(const string& coin, PreciseNumber quantity, PreciseNumber locked);

	vector<shared_ptr<BinanceWebsocket>> m_tickers;
	shared_ptr<BinanceOrders> m_orders;

public:
	BinanceTrader(const string& apiKey, const string& apiSecret);

	virtual void connect();
	virtual void disconnect();

	bool cancelOrder(const OrderInfo& order);
	bool createOrder(const OrderInfo& order);
	void createMultipleOrders(const vector<OrderInfo*>& orders, vector<bool>& results, size_t count);

	PreciseNumber getBalanceSummary(bool old = false);
	shared_ptr<BookLoader> getBook(const string& symbol1, const string& symbol2);
	shared_ptr<BookLoader> getBook(const string& pair);

	PreciseNumber getCoinBalance(const string& coin);
	bool lockCoinBalanceInNewOrders(const string& coin, PreciseNumber quantity);
	bool unlockCoinBalanceInNewOrders(const string& coin, PreciseNumber quantity);
	void setCoinBalanceReserved(const string& coin, PreciseNumber quantity);

	PreciseNumber getComissionDivider() { return m_comissionDivider; };
	PreciseNumber getComissionBase() { return m_comissionBase; };
	PreciseNumber getComissionMultiplier() { return m_comissionMultiplier; };

	PairInfo getPairInfo(const string& pair);
	map<string, vector<string>> getPairsBySymbol();
	
	PreciseNumber getSymbolCostAs(const string& sourceSymbol, PreciseNumber quantity, const string& targetSymbol);
	PreciseNumber getSymbolMinLotSize(const string& pair);
	PreciseNumber getSymbolNotion(const string& pair);

	int64_t getTimestamp();

	bool hasPair(const string& pair);

	shared_ptr<BinanceOrders> orders()
	{
		return m_orders;
	}

	void registerBalanceChangedHandler(OnBalanceChanged handler);
	void registerBookListChangedHandler(OnBookListChanged handler);
	void registerOrderStatusChangedHandler(OnOrderStatusChanged handler);

	static atomic<uint64_t> m_bookHits;
};

