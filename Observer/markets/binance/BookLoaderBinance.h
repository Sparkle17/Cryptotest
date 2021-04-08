#pragma once
#include "../BookLoader.h"
#include <string>
#include <vector>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include "BinanceTrader.h"
#include "BinanceWebsocket.h"
#include "../../json/json.h"

using namespace std;

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = boost::asio::ip::tcp;

class BookLoaderBinance :
    public BookLoader
{
private:
	//websocket::stream<beast::ssl_stream<tcp::socket>> m_websocket;
	//beast::flat_buffer m_buffer;

	//http::request<http::empty_body> m_apiheader;
	//beast::ssl_stream<tcp::socket> m_apisocket;
	//beast::flat_buffer m_apibuffer;
	//http::response<http::string_body> m_apidata;

	//uint64_t m_lastUpdateId;
	//bool m_updateDone;
	//vector<string> m_cache;

	//void bookConnectHandler(const boost::system::error_code& err);
	//void bookHandshakeHandler(const boost::system::error_code& err);
	//void bookReadHandler(const boost::system::error_code& err, size_t bytes_transferred);
	//void bookWriteHandler(const boost::system::error_code& err, size_t bytes_transferred);

	//void closeHandler(const boost::system::error_code& err);
	//void readNext();

	//void parseBook(const string& data);
	//void parseUpdate(const string& data);
	//void parseOrders(JSONLoader& json, map<PreciseNumber, PreciseNumber>& orders);

	shared_ptr<BinanceWebsocket> m_ticker;
	uint64_t m_lastOrderbookUpdateId;

	void parseTicker(const string& data, bool success);

	//void streamConnectHandler(const boost::system::error_code& err, tcp::resolver::results_type::endpoint_type ep);
	//void streamHandshakeHandler(const boost::system::error_code& err, string wshost);
	//void streamHandshakeWsHandler(const boost::system::error_code& err);
	//void streamReadHandler(const boost::system::error_code& err, size_t bytes_transferred);

public:
	BookLoaderBinance(const PairInfo& info);

	virtual void connect() override;
	virtual void disconnect() override;

	void updateTicker(uint64_t id, PreciseNumber bestBid, PreciseNumber bestBidQuantity, PreciseNumber bestAsk, PreciseNumber bestAskQuantity);
};
