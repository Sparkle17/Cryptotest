#include "BinanceTrader.h"
#include <filesystem>
#include <iostream>

#include <boost/algorithm/string.hpp>

#include "BookLoaderBinance.h"
#include "../../json/json.h"
#include "../../Logger.h"
#include "../OrderInfo.h"

atomic<uint64_t> BinanceTrader::m_bookHits;
const string BinanceTrader::dataPath = "data\\markets\\binance\\";

namespace BTBalance {
	const string balance{ "balance" };
	const string locked{ "locked" };
	const string quantity{ "quantity" };
	const string reserved{ "reserved" };
	const string symbol{ "symbol" };
};

namespace BTFiles {
	const string balance{ "balance.txt" };
};

BinanceTrader::BinanceTrader(const string& apiKey, const string& apiSecret)
	: m_timestampDelta(0),
	m_infoRequested(false),
	m_baseSymbols({ "USDT", "BUSD", "BTC" }),
	m_APIKey(apiKey),
	m_APISecret(apiSecret)
{
	filesystem::create_directories(dataPath);

	PreciseNumber comission("0.00075");
	m_comissionMultiplier = PreciseNumber(1);
	m_comissionDivider = PreciseNumber(1);
	m_comissionBase = PreciseNumber(1) - comission;

	m_userStream = make_shared<UserStreamCreator>();
	m_orders = make_shared<BinanceOrders>();

	loadBalance();
	//setCoinBalanceReserved("BNB", PreciseNumber("0.05"));
	Logger::d("BNB reserved: " + m_balance["BNB"].balanceReserved.to_string());
}

void BinanceTrader::connect()
{
	unique_lock<shared_mutex> lock(m_APIAccess);
	for (int i = 0; i < maxApi; i++) {
		auto api = make_shared<BinanceApi>(m_APIKey, m_APISecret, shared_from_this());
		api->connect(bind(&BinanceTrader::onApiConnected, shared_from_this(), placeholders::_1));
		m_apis.push_back(move(api));
	}
	lock.unlock();

	m_connected = true;
	thread th(bind(&BinanceTrader::refreshApi, shared_from_this()));
	m_apiRefresher.swap(th);

	m_orders->connect(shared_from_this());
}

bool BinanceTrader::cancelOrder(const OrderInfo& order)
{
	auto api = getApi();
	try {
		if (api.get()) {
			auto obj = shared_from_this();
			auto handler = [order, obj, api](const string& data, bool success, HttpResponse response) {
				try {
					Logger::d("BinanceTrader::cancelOrder, status: " + to_string(response.result_int()));
					if (response.result_int() != 200) {
						obj->onOrderStatusChanged(order.getIdString(), "NEW", "NEW", true);
						if (data.size() == 0) {
							Logger::e("API connection error");
							api->disconnect();
						}
					}
				}
				catch (exception& e) {
					Logger::e("BinanceTrader::cancelOrder exception: " + string(e.what()));
				}
				catch (...) {
					Logger::e("BinanceTrader::cancelOrder unknown exception");
				}
			};
			api->cancelOrder(order.getPair(), order.getIdString(), handler);
			Logger::d("BinanceTrader::cancelOrder: order canceled: " + order.getFullInfo());
			return true;
		}
		else return false;
	}
	catch (...) {
	}
	api->release();
	return false;
}

void BinanceTrader::createMultipleOrders(const vector<OrderInfo*>& orders, vector<bool>& results, size_t count)
{
	vector<shared_ptr<BinanceApi>> freeApis;

	try {
		for (size_t i = 0; i < count; i++) {
			auto api = getApi();
			if (api.get())
				freeApis.push_back(move(api));
			else break;
		}
		if (freeApis.size() == count) {
			for (size_t i = 0; i < count; i++) {
				results[i] = createOrderInternal(*orders[i], freeApis[i]);
				if (!results[i]) freeApis[i]->release();
			}
			Logger::d("BinanceTrader::createMultipleOrders completed");
			return;
		}
	}
	catch (...)	{
		Logger::e("BinanceTrader::createMultipleOrders exception caught!");
	}

	for (auto &api : freeApis)
		api->release();
}

bool BinanceTrader::createOrder(const OrderInfo& order)
{
	auto api = getApi();
	try {
		if (api.get()) {
			bool result = createOrderInternal(order, api);
			if (result) return true;
		}
		else 
			return false;
	}
	catch (exception) {
	}
	api->release();
	return false;
}

// Ѕлокирует баланс дл€ ордера лишь временно, до поступлени€ с сервера актуальной информации о балансе и блокировке
bool BinanceTrader::createOrderInternal(const OrderInfo& order, shared_ptr<BinanceApi> api)
{
	if (lockCoinBalanceInNewOrders(order.sellSymbol(), order.sellValue())) {
		Logger::d("BinanceTrader::createOrderInternal: placing order " + order.sellSymbol() + " -> " + order.buySymbol());
		auto data = order.getOrderData();
		auto obj = shared_from_this();
		auto handler = [order, obj, api](const string& data, bool success, HttpResponse response) {
			try {
				Logger::d("status: " + to_string(response.result_int()));
				Logger::d(data);
				if (response.result_int() != 200 || data.size() == 0) {
					obj->unlockCoinBalanceInNewOrders(order.sellSymbol(), order.sellValue());
					obj->onOrderStatusChanged(order.getIdString(), "REJECTED", "REJECTED", false);
					if (data.size() == 0) {
						Logger::e("API connection error");
						api->disconnect();
					}
				}
			}
			catch (exception &e) {
				Logger::e("createOrderInternal exception: " + string(e.what()));
			}
			catch (...) {
				Logger::e("createOrderInternal unknown exception");
			}
		};
		api->createOrder(data, handler);
		Logger::d("BinanceTrader::createOrderInternal: order started " + order.getFullInfo());
		return true;
	}
	return false;
}

void BinanceTrader::disconnect() 
{
	m_connected = false;
	m_bookLoader.join();

	m_orders->disconnect();
	m_userStream->disconnect();
	saveBalance();

	unique_lock<shared_mutex> lock(m_booksAccess);
	for (auto book : m_books)
		book.second->disconnect();
	lock.release();

	for (auto& ticker : m_tickers)
		ticker->disconnect();

	for (auto api : m_apis)
		api->disconnect();
}

shared_ptr<BinanceApi> BinanceTrader::getApi()
{
	shared_lock<shared_mutex> lock(m_APIAccess);
	for (auto api : m_apis)
		if (api->try_lock())
			return api;
	Logger::e("not enough api handlers!");
	return shared_ptr<BinanceApi>();
}

PreciseNumber BinanceTrader::getBalanceSummary(bool old)
{
	shared_lock<shared_mutex> lock(m_balanceAccess);
	PreciseNumber result;
	for (auto& bal : m_balance) {
		auto cost = getSymbolCostAs(bal.first, old ? bal.second.balanceStart : (bal.second.balance + bal.second.balanceLocked), "USDT");
		result += cost;
	}
	return result;
}

shared_ptr<BookLoader> BinanceTrader::getBook(const string& symbol1, const string& symbol2) 
{
	shared_lock<shared_mutex> lock(m_booksAccess);
	auto it = m_books.find(symbol1 + symbol2);
	if (it != m_books.end()) return it->second;
	it = m_books.find(symbol2 + symbol1);
	if (it != m_books.end()) return it->second;
	return shared_ptr<BookLoader>();
}

shared_ptr<BookLoader> BinanceTrader::getBook(const string& pair)
{
	shared_lock<shared_mutex> lock(m_booksAccess);
	auto it = m_books.find(pair);
	if (it != m_books.end()) return it->second;
	return shared_ptr<BookLoader>();
}

// возвращает баланс доступный дл€ торгов
PreciseNumber BinanceTrader::getCoinBalance(const string& coin)
{
	shared_lock<shared_mutex> lock(m_balanceAccess);
	auto it = m_balance.find(coin);
	if (it != m_balance.end()) {
		auto result = it->second.balance - it->second.balanceReserved;
		return result > 0 ? result : 0;
	}
	return PreciseNumber();
}

PairInfo BinanceTrader::getPairInfo(const string& pair)
{
	auto it = m_info.find(pair);
	if (it != m_info.end())
		return it->second;
	return PairInfo();
}

map<string, vector<string>> BinanceTrader::getPairsBySymbol()
{
	shared_lock<shared_mutex> lock(m_booksAccess);
	map<string, vector<string>> pairs;
	for (auto& info : m_info) {
		auto it = pairs.find(info.second.base);
		if (it == pairs.end()) pairs[info.second.base] = { info.first };
		else it->second.push_back(info.first);

		it = pairs.find(info.second.quoted);
		if (it == pairs.end()) pairs[info.second.quoted] = { info.first };
		else it->second.push_back(info.first);
	}
	return pairs;
}

PreciseNumber BinanceTrader::getSymbolCostAs(const string& sourceSymbol, PreciseNumber quantity, const string& targetSymbol)
{
	if (quantity == 0 || sourceSymbol == targetSymbol)
		return quantity;
	shared_lock<shared_mutex> lock(m_booksAccess);

	static auto finder = [this](const string& sourceSymbol, PreciseNumber quantity, const string& targetSymbol) -> PreciseNumber
	{
		auto it = m_books.find(sourceSymbol + targetSymbol);
		if (it != m_books.end())
			return quantity * it->second->bestBuy().first;
		else {
			it = m_books.find(targetSymbol + sourceSymbol);
			if (it != m_books.end()) {
				PreciseNumber cost = it->second->bestSell().first;
				return cost == 0 ? 0 : quantity / cost;
			}
		}
		return 0;
	};

	auto result = finder(sourceSymbol, quantity, targetSymbol);
	for (const auto& symbol : m_baseSymbols)
		if (result == 0) {
			if (targetSymbol != symbol)
				result = finder(symbol, finder(sourceSymbol, quantity, symbol), targetSymbol);
		}
		else break;
	return result;
}

PreciseNumber BinanceTrader::getSymbolMinLotSize(const string& pair)
{
	auto it = m_info.find(pair);
	if (it != m_info.end())
		return it->second.minLotSize;
	return -1;
}

PreciseNumber BinanceTrader::getSymbolNotion(const string& pair) {
	auto it = m_info.find(pair);
	if (it != m_info.end())
		return it->second.notion;
	return PreciseNumber();
}

int64_t BinanceTrader::getSystemTimeAsUnixTime()
{
	const int64_t UNIX_TIME_START = 0x019DB1DED53E8000; //January 1, 1970 (start of Unix epoch) in "ticks"
	const int64_t TICKS_PER_MSEC = 10000;

	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);

	LARGE_INTEGER li;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	return (li.QuadPart - UNIX_TIME_START) / TICKS_PER_MSEC;
}

int64_t BinanceTrader::getTimestamp()
{
	return getSystemTimeAsUnixTime() + m_timestampDelta;
}

bool BinanceTrader::hasPair(const string& pair) {
	return m_info.find(pair) != m_info.end();
}

void BinanceTrader::loadBalance()
{
	try {
		auto data = json::load(dataPath + BTFiles::balance);
		auto balance = move(data[BTBalance::balance]);
		for (size_t i = 0; i < balance.size(); i++) {
			auto item = move(balance[i]);

			string symbol = item[BTBalance::symbol].getString();
			PreciseNumber quantity = PreciseNumber(item[BTBalance::quantity].getString());
			PreciseNumber locked = PreciseNumber(item[BTBalance::locked].getString());
			PreciseNumber reserved = PreciseNumber(item[BTBalance::reserved].getString());
			m_balance[symbol] = { quantity + locked, quantity, locked, 0, reserved };
		}
	}
	catch (exception) {
		Logger::e("can\'t load balance");
	}
}

// работает в отдельном потоке
// полные стаканы отключены
void BinanceTrader::loadBooks() {
	// TODO: вернуть на 50 если подключаютс€ полные стаканы
	const int64_t minInterval = 5;

	auto start = chrono::system_clock::now();
	int count = 0;
	for (auto &info : m_info) {
		unique_lock<shared_mutex> lock(m_booksAccess);
		auto book = make_shared<BookLoaderBinance>(info.second);
		book->connect();
		m_books[info.first] = move(book);
		lock.unlock();

		count++;
		auto elapsed = chrono::system_clock::now() - start;
		int64_t delay = minInterval * count - chrono::duration_cast<chrono::milliseconds>(elapsed).count();
		if (delay > 0)
			this_thread::sleep_for(chrono::milliseconds(delay));
	}
	onBooksListChanged();

	auto obj = shared_from_this();
	auto handler = [obj](const string& data, bool success)
	{
		BinanceTrader::m_bookHits++;
		try {
			JSONLoader json(data);
			json.skipTo("\"data\":");
			json.skip("{\"u\":");
			uint64_t id = json.getNumber();
			json.skip(",\"s\":");
			string symbol = json.getString();

			auto book = obj->m_books.find(symbol);
			if (book != obj->m_books.end()) {
				json.skip(",\"b\":");
				auto bestBid = json.getPreciseNumber();
				json.skip(",\"B\":");
				auto bestBidQuantity = json.getPreciseNumber();
				json.skip(",\"a\":");
				auto bestAsk = json.getPreciseNumber();
				json.skip(",\"A\":");
				auto bestAskQuantity = json.getPreciseNumber();
				book->second->updateTicker(id, bestBid, bestBidQuantity, bestAsk, bestAskQuantity);
			}
		}
		catch (exception const&) {
			Logger::e("parsing failed: " + data);
		}
	};

	shared_lock<shared_mutex> lock(m_booksAccess);
	auto it = m_books.begin();
	while (it != m_books.end()) {
		string addr = "/stream?streams=";
		int i = 0;
		while (it != m_books.end() && i < 20) {
			if (i > 0) addr += "/";
			addr += boost::to_lower_copy(it->first) + "@bookTicker";
			i++;
			it++;
		}
		if (i > 0) {
			auto ticker = make_shared<BinanceWebsocket>(addr, handler);
			ticker->connect();
			m_tickers.push_back(move(ticker));
		}
	}
	Logger::i("book tickers connected: " + to_string(m_tickers.size()));
}

bool BinanceTrader::lockCoinBalanceInNewOrders(const string& coin, PreciseNumber quantity)
{
	lock_guard<shared_mutex> lock(m_balanceAccess);
	auto it = m_balance.find(coin);
	if (it != m_balance.end()) {
		if (it->second.balance >= quantity) {
			it->second.balance -= quantity;
			it->second.balanceInNewOrders += quantity;
			Logger::d("BinanceTrader::lockCoinBalanceInOrders: coin balance locked " + coin + " - " + quantity.to_string() + " (left: " + it->second.balance.to_string() + ")");
			return true;
		}
	}
	return false;
}

void BinanceTrader::onApiConnected(bool success) {
	if (success) {
		m_userStream->connect(getApi(), bind(&BinanceTrader::userStreamHandler, shared_from_this(), placeholders::_1, placeholders::_2));
		refreshTime();
	}
}

void BinanceTrader::onBalanceChanged(const string& coin)
{
	shared_lock<shared_mutex> lock(m_handlersAccess);
	for (auto handler : m_onBalanceChanged)
		handler(coin);
}

void BinanceTrader::onBalanceInfo(const string& data, bool success, HttpResponse response)
{
	try {
		JSONLoader json(data);
		auto obj = json.parse();
		auto balances = move(obj["balances"]);
		for (size_t i = 0; i < balances.size(); i++) {
			auto item = move(balances[i]);
			setCoinBalance(item["asset"].getString(), PreciseNumber(item["free"].getString()), PreciseNumber(item["locked"].getString()));
		}
	}
	catch (exception& e) {
		Logger::e("Error parsing account balance info: " + string(e.what()));
	}
}

void BinanceTrader::onBooksListChanged() {
	shared_lock<shared_mutex> lock(m_handlersAccess);
	for (auto handler : m_onBookListChanged)
		handler();
}

void BinanceTrader::onOrderStatusChanged(const string& orderUserId, const string& executionType, const string& currentStatus, bool inBook)
{
	shared_lock<shared_mutex> lock(m_handlersAccess);
	for (auto handler : m_onOrderStatusChanged)
		handler(orderUserId, executionType, currentStatus, inBook);
}

void BinanceTrader::onTradeInfo(string data, bool success) {
	JSONLoader json(data);
	auto obj = json.parse();
	auto dataSymbols = move(obj["symbols"]);
	for (size_t i = 0; i < dataSymbols.size(); i++) {
		auto symbol = move(dataSymbols[i]);
		auto pairName = symbol["symbol"].getString();
		auto status = symbol["status"].getString();
		auto baseName = symbol["baseAsset"].getString();
		auto quoteName = symbol["quoteAsset"].getString();
		auto isSpotTradingAllowed = symbol["isSpotTradingAllowed"].getBoolean();

		// пропускаем пары маржинальной торговли (UP & DOWN)
		if (isSpotTradingAllowed && status == "TRADING") {
			PreciseNumber minLotSize = 0;
			PreciseNumber notion = 0;

			auto filters = move(symbol["filters"]);
			for (size_t j = 0; j < filters.size(); j++) {
				auto filter = move(filters[j]);
				string filterName = filter["filterType"].getString();
				if (filterName == "LOT_SIZE")
					minLotSize = PreciseNumber(filter["stepSize"].getString());
				else if (filterName == "MIN_NOTIONAL")
					notion = PreciseNumber(filter["minNotional"].getString());
			}

			m_info[pairName] = { baseName, quoteName, minLotSize, notion };
		}
	}

	// убираем все символы, у который только одна пара (дл€ текущего арбитража не подход€т)
	map<string, int> symbols;
	for (auto& info : m_info) {
		auto it = symbols.find(info.second.base);
		if (it == symbols.end()) symbols[info.second.base] = 1;
		else symbols[info.second.base]++;

		it = symbols.find(info.second.quoted);
		if (it == symbols.end()) symbols[info.second.quoted] = 1;
		else symbols[info.second.quoted]++;
	}
	erase_if(m_info, [&symbols](const auto& item) {
			auto const& [key, value] = item;
			return symbols[value.base] == 1 || symbols[value.quoted] == 1;
		});

	Logger::i("total symbols: " + to_string(symbols.size()));
	Logger::i("total trade pairs found: " + to_string(m_info.size()));
	thread th(bind(&BinanceTrader::loadBooks, shared_from_this()));
	m_bookLoader.swap(th);
}

void BinanceTrader::refreshApi()
{
	this_thread::sleep_for(std::chrono::seconds(10));

	int count = 0;
	while (m_connected) {
		count++;
		if (count >= 30) {
			auto api = getApi();
			if (api)
				m_userStream->refresh(api);
			count = 0;
		}

		auto obj = shared_from_this();
		for (auto api : m_apis) 
			if (api->getIdleTime() > chrono::milliseconds(25000)) {
				if (api->try_lock()) {
					api->refresh([api, obj](const string& data, bool success, HttpResponse response)
						{
							if (success && data.size() != 0)
								obj->updateServerTime(data);
							else
								api->disconnect();
						});
					this_thread::sleep_for(std::chrono::milliseconds(100));
				}
			}

		unique_lock<shared_mutex> lock(m_APIAccess);
		m_apis.erase(remove_if(m_apis.begin(), m_apis.end(), [](auto item) { return !item->isConnected(); }), m_apis.end());
		while (m_apis.size() < maxApi) {
			auto api = make_shared<BinanceApi>(m_APIKey, m_APISecret, shared_from_this());
			api->connect();
			m_apis.push_back(move(api));
		}
		lock.unlock();

		this_thread::sleep_for(std::chrono::seconds(30));
	}
}

void BinanceTrader::registerBalanceChangedHandler(OnBalanceChanged handler)
{
	unique_lock<shared_mutex> lock(m_handlersAccess);
	m_onBalanceChanged.push_back(handler);
}

void BinanceTrader::registerBookListChangedHandler(OnBookListChanged handler)
{
	unique_lock<shared_mutex> lock(m_handlersAccess);
	m_onBookListChanged.push_back(handler);
}

void BinanceTrader::registerOrderStatusChangedHandler(OnOrderStatusChanged handler)
{
	unique_lock<shared_mutex> lock(m_handlersAccess);
	m_onOrderStatusChanged.push_back(handler);
}

void BinanceTrader::refreshData() 
{
	while (!m_balanceRequested.load()) {
		requestTradeInfo();
		m_orders->loadOrders();
		requestBalance();
		this_thread::sleep_for(500ms);
	}
}

void BinanceTrader::refreshTime()
{
	auto api = getApi();
	if (api) {
		bool b = false;
		if (m_timeRefreshed.compare_exchange_strong(b, true)) {
			auto obj = shared_from_this();
			api->refresh([api, obj](const string& data, bool success, HttpResponse response)
				{
					if (success && data.size() != 0)
						obj->updateServerTime(data);
					else
						api->disconnect();
					thread(bind(&BinanceTrader::refreshData, obj)).detach();
				});
		}
		else
			api->release();
	}
}

void BinanceTrader::requestBalance()
{
	auto api = getApi();
	if (api) {
		bool b = false;
		if (m_balanceRequested.compare_exchange_strong(b, true))
			api->getAccountInfo(bind(&BinanceTrader::onBalanceInfo, shared_from_this(), placeholders::_1, placeholders::_2, placeholders::_3));
		else
			api->release();
	}
}

void BinanceTrader::requestTradeInfo() {
	auto api = getApi();
	if (api) {
		bool b = false;
		if (m_infoRequested.compare_exchange_strong(b, true))
			api->getExchangeData(bind(&BinanceTrader::onTradeInfo, shared_from_this(), placeholders::_1, placeholders::_2));
		else
			api->release();
	}
}

void BinanceTrader::saveBalance()
{
	auto data = json::createObject();
	auto balance = json::createArray();
	for (auto& b : m_balance) 
		if (b.second.balance > 0 || b.second.balanceLocked > 0) {
			auto item = json::createObject();
			item[BTBalance::symbol] = json::createString(b.first);
			item[BTBalance::quantity] = json::createString(b.second.balance.to_string());
			item[BTBalance::locked] = json::createString(b.second.balanceLocked.to_string());
			item[BTBalance::reserved] = json::createString(b.second.balanceReserved.to_string());
			balance.add(move(item));
		}
	data[BTBalance::balance] = move(balance);
	json::save(dataPath + BTFiles::balance, move(data));
}

void BinanceTrader::setCoinBalance(const string& coin, PreciseNumber quantity, PreciseNumber locked)
{
	unique_lock<shared_mutex> lock(m_balanceAccess);
	auto it = m_balance.find(coin);
	if (it == m_balance.end()) {
		if (quantity == 0 && locked == 0)
			return;
		m_balance[coin] = { 0, quantity, locked, 0, 0 };
	}
	else {
		if (it->second.balance == quantity && it->second.balanceLocked == locked)
			return;
		it->second.balance = quantity;
		it->second.balanceLocked = locked;
		it->second.balanceInNewOrders = 0;
	}
	lock.unlock();

	onBalanceChanged(coin);
	Logger::d("balance changed: " + coin + " = " + quantity.to_string() + " (locked " + locked.to_string() + ")");
}

void BinanceTrader::setCoinBalanceReserved(const string& coin, PreciseNumber quantity)
{
	unique_lock<shared_mutex> lock(m_balanceAccess);
	auto it = m_balance.find(coin);
	if (it == m_balance.end())
		m_balance[coin] = { 0, 0, 0, 0, quantity };
	else it->second.balanceReserved = quantity;
}

bool BinanceTrader::unlockCoinBalanceInNewOrders(const string& coin, PreciseNumber quantity)
{
	lock_guard<shared_mutex> lock(m_balanceAccess);
	auto it = m_balance.find(coin);
	if (it != m_balance.end()) {
		if (it->second.balanceInNewOrders >= quantity) {
			it->second.balance += quantity;
			it->second.balanceInNewOrders -= quantity;
			return true;
		}
	}
	return false;
}

void BinanceTrader::updateServerTime(const string& data)
{
	try {
		JSONLoader json(data);
		auto obj = json.parse();
		auto serverTime = obj["serverTime"].getInteger();
		auto localTime = getSystemTimeAsUnixTime();
		m_timestampDelta = serverTime - localTime;
	}
	catch (exception& e) {
		Logger::e("error parsing server time info: " + string(e.what()));
	}
}

void BinanceTrader::UserStreamCreator::connect(shared_ptr<BinanceApi> api, WebsocketRequestHandler handler)
{
	if (api) {
		bool b = false;
		if (m_connected.compare_exchange_strong(b, true)) {
			auto obj = shared_from_this();
			api->getUserStream([obj, handler](const string& data, bool success, HttpResponse response)
				{
					Logger::i(data);
					if (success) {
						JSONLoader json(data);
						json.skipTo("\"listenKey\":");
						obj->m_userStreamKey = json.getString();
						obj->m_userDataStream = make_shared<BinanceWebsocket>("/ws/" + obj->m_userStreamKey, handler);
						obj->m_userDataStream->connect();
					}
				});
		}
		else
			api->release();
	}
}

void BinanceTrader::UserStreamCreator::disconnect()
{
	if (m_userDataStream)
		m_userDataStream->disconnect();
	m_connected = false;
}

// TODO: добавить обновление и самого вебсокета
void BinanceTrader::UserStreamCreator::refresh(shared_ptr<BinanceApi> api)
{
	if (m_connected)
		api->refreshUserStream(m_userStreamKey, [](string data, bool, HttpResponse)
			{
				Logger::d("user stream refresh: " + data);
			});
}

void BinanceTrader::userStreamHandler(string data, bool success)
{
	if (success) {
		try {
			Logger::d(data);
			JSONLoader json(data);
			auto result = json.parse();
			string event = result["e"].getString();
			if (event == "outboundAccountPosition") {
				// balanse changed
				auto assets = move(result["B"]);
				for (size_t i = 0; i < assets.size(); i++) {
					auto asset = move(assets[i]);
					setCoinBalance(asset["a"].getString(), PreciseNumber(asset["f"].getString()), PreciseNumber(asset["l"].getString()));
				}
			}
			else if (event == "executionReport") {
				// order changed
				auto executionType = result["x"].getString();
				auto orderId = executionType == "CANCELED" ? result["C"].getString() : result["c"].getString();
				onOrderStatusChanged(orderId, executionType, result["X"].getString(), result["w"].getBoolean());
			}
			else if (event == "balanceUpdate")
			{
			}
			else
				Logger::e("unknown user event type: " + event);
		}
		catch (...) {
			Logger::e("exception while parsing user data: " + data);
		}
	}
}
