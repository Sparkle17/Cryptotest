#pragma once
#include <chrono>
#include <memory>
#include <string>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include "..\..\Network.h"

#include "..\..\PreciseNumber.h"

using namespace std;

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

struct OrderData
{
    string symbol;
    bool buy;
    PreciseNumber quantity;
    PreciseNumber price;
    string orderId;
};

class BinanceTrader;

class BinanceApi: public enable_shared_from_this<BinanceApi>
{
private:
    weak_ptr<BinanceTrader> m_trader;
    string m_apiKey;
    string m_apiSecret;
    chrono::system_clock::time_point m_lastRequestTime;

    atomic<bool> m_connected;
    atomic<bool> m_ready;

    http::request<http::string_body> m_apiheader;
    beast::ssl_stream<tcp::socket> m_apisocket;
    beast::flat_buffer m_apibuffer;
    http::response<http::string_body> m_apidata;

    ConnectHandler m_onConnect;
    HttpRequestHandler m_onResponse;

    void closeHandler(const boost::system::error_code& err);
    void connectHandler(const boost::system::error_code& err);
    void handshakeHandler(const boost::system::error_code& err);
    void readHandler(const boost::system::error_code& err, size_t bytes_transferred);
    void writeHandler(const boost::system::error_code& err, size_t bytes_transferred);

    void onConnect(const boost::system::error_code& err);

    string calculateHmacSHA256(const string& msg);
    int64_t getTimestamp();

    void sendRequest(boost::beast::http::verb method, const string& request, bool apiKey = false, const string& body = "");

public:
    BinanceApi(const string& apiKey, const string& apiSecret, weak_ptr<BinanceTrader> trader);

    virtual void connect(ConnectHandler handler = NULL);
    virtual void disconnect();
    bool isConnected() { return m_connected; }

    void cancelOrder(const string& symbol, const string& orderId, HttpRequestHandler handler = NULL);
    void createOrder(const OrderData& data, HttpRequestHandler handler = NULL);
    void getAccountInfo(HttpRequestHandler handler);
    void getExchangeData(HttpRequestHandler handler);
    chrono::milliseconds getIdleTime();
    void getOrdersData(HttpRequestHandler handler);
    void getUserStream(HttpRequestHandler handler);
    void refresh(HttpRequestHandler handler);
    void refreshUserStream(const string& streamKey, HttpRequestHandler handler);

    bool isReady() { return m_ready;  }
    void release() { m_ready = true; }
    bool try_lock() { 
        bool expected = true;
        if (m_ready.compare_exchange_strong(expected, false))
            return expected;
        return false;
    }
};
