#include "BinanceApi.h"
#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

#include <openssl/sha.h>
#include <openssl/hmac.h>

#include "BinanceTrader.h"
#include "..\..\Logger.h"
#include "..\..\Network.h"

using namespace std;

namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

BinanceApi::BinanceApi(const string& apiKey, const string& apiSecret, weak_ptr<BinanceTrader> trader)
    : m_trader(trader),
    m_apiKey(apiKey),
    m_apiSecret(apiSecret),
    m_apisocket(Network::getContext(), Network::getSSLContext()),
    m_connected(false),
    m_ready(false)
{
}

string BinanceApi::calculateHmacSHA256(const string& msg)
{
    array<unsigned char, EVP_MAX_MD_SIZE> hash;
    unsigned int hashLen;
    HMAC(EVP_sha256(), m_apiSecret.data(), static_cast<int>(m_apiSecret.size()), reinterpret_cast<unsigned char const*>(msg.data()), msg.size(), hash.data(), &hashLen);

    string result(hashLen * 2, '0');
    for (size_t i = 0; i < hashLen; i++) {
        uint8_t c = hash[i] >> 4;
        result[i * 2] = (c > 9 ? ('a' - 10 + c) : ('0' + c));
        c = hash[i] & 0xf;
        result[i * 2 + 1] = (c > 9 ? ('a' - 10 + c) : ('0' + c));
    }
    return result;
}

void BinanceApi::closeHandler(const boost::system::error_code& err)
{
}

void BinanceApi::connect(ConnectHandler handler) {
    m_onConnect = handler;
    auto apiaddr = Network::binance_api();
    net::async_connect(get_lowest_layer(m_apisocket), apiaddr.resolved, bind(&BinanceApi::connectHandler, shared_from_this(), placeholders::_1));
}

void BinanceApi::connectHandler(const boost::system::error_code& err)
{
    if (!err)
        m_apisocket.async_handshake(ssl::stream_base::client, bind(&BinanceApi::handshakeHandler, shared_from_this(), placeholders::_1));
    else {
        Logger::e("BinanceApi::connectHandler error=" + err.message());
        onConnect(err);
    }
}

void BinanceApi::cancelOrder(const string& symbol, const string& orderId, HttpRequestHandler handler)
{
    m_onResponse = handler;
    string request = "symbol=" + symbol + "&origClientOrderId=" + orderId + "&newClientOrderId=" + orderId + "&recvWindow=5000&timestamp=" + to_string(getTimestamp());
    string hmac = calculateHmacSHA256(request);
    sendRequest(http::verb::delete_, "/api/v3/order?" + request + "&signature=" + hmac, true);
}

void BinanceApi::createOrder(const OrderData& data, HttpRequestHandler handler)
{
    m_onResponse = handler;
    string request = "symbol=" + data.symbol + "&side=" + (data.buy ? "BUY" : "SELL") + "&type=LIMIT&timeInForce=GTC&quantity=" + data.quantity.to_string() + 
        "&price=" + data.price.to_string() + "&newClientOrderId=" + data.orderId + "&recvWindow=5000&timestamp=" + to_string(getTimestamp());
    string hmac = calculateHmacSHA256(request);
    sendRequest(http::verb::post, "/api/v3/order?" + request + "&signature=" + hmac, true);
}

void BinanceApi::disconnect()
{
    m_connected = false;
    m_ready = false;
    m_apisocket.async_shutdown(bind(&BinanceApi::closeHandler, shared_from_this(), placeholders::_1));
}

void BinanceApi::getAccountInfo(HttpRequestHandler handler) {
    m_onResponse = handler;
    string request = "timestamp=" + to_string(getTimestamp());
    string hmac = calculateHmacSHA256(request);
    sendRequest(http::verb::get, "/api/v3/account?" + request + "&signature=" + hmac, true);
}

void BinanceApi::getExchangeData(HttpRequestHandler handler) {
    m_onResponse = handler;
    sendRequest(http::verb::get, "/api/v3/exchangeInfo");
}

chrono::milliseconds BinanceApi::getIdleTime()
{
    return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - m_lastRequestTime);
}

void BinanceApi::getOrdersData(HttpRequestHandler handler) {
    m_onResponse = handler;
    string request = "timestamp=" + to_string(getTimestamp());
    string hmac = calculateHmacSHA256(request);
    sendRequest(http::verb::get, "/api/v3/openOrders?" + request + "&signature=" + hmac, true);
}

int64_t BinanceApi::getTimestamp()
{
    auto trader = m_trader.lock();
    if (trader)
        return trader->getTimestamp();
    else
        throw(runtime_error("BinanceTrader already closed"));
}

void BinanceApi::getUserStream(HttpRequestHandler handler)
{
    m_onResponse = handler;
    sendRequest(http::verb::post, "/api/v3/userDataStream", true);
}

void BinanceApi::handshakeHandler(const boost::system::error_code& err)
{
    if (err)
        Logger::e("handshakeHandler error=" + err.message());

    if (!err) {
        m_connected = true;
        m_ready = true;
    }
    onConnect(err);
}

void BinanceApi::onConnect(const boost::system::error_code& err)
{
    if (m_onConnect) m_onConnect(!err);
}

void BinanceApi::readHandler(const boost::system::error_code& err, size_t bytes_transferred)
{
    if (m_onResponse) m_onResponse(m_apidata.body(), !err, m_apidata);
    if (m_apidata.body().size() == 0)
        disconnect();
    else
        release();
}

void BinanceApi::refresh(HttpRequestHandler handler)
{
    m_onResponse = handler;
    sendRequest(http::verb::get, "/api/v3/time");
}

void BinanceApi::refreshUserStream(const string& streamKey, HttpRequestHandler handler)
{
    m_onResponse = handler;
    sendRequest(http::verb::put, "/api/v3/userDataStream?listenKey=" + streamKey, true);
}

void BinanceApi::sendRequest(boost::beast::http::verb method, const string& request, bool apiKey, const string& body)
{
    m_ready = false;
    m_lastRequestTime = chrono::system_clock::now();
    auto apiaddr = Network::binance_api();
    m_apiheader.version(11);
    m_apiheader.method(method);
    m_apiheader.target(request);
    m_apiheader.set(http::field::host, apiaddr.host);
    m_apiheader.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    if (apiKey)
        m_apiheader.set("X-MBX-APIKEY", m_apiKey);
    if (body.length() > 0) {
        m_apiheader.set(beast::http::field::content_type, "application/x-www-form-urlencoded");
        m_apiheader.body() = body;
        m_apiheader.prepare_payload();
    }
    http::async_write(m_apisocket, m_apiheader, bind(&BinanceApi::writeHandler, shared_from_this(), placeholders::_1, placeholders::_2));
}

void BinanceApi::writeHandler(const boost::system::error_code& err, size_t bytes_transferred)
{
    m_apidata = {};
    http::async_read(m_apisocket, m_apibuffer, m_apidata, bind(&BinanceApi::readHandler, shared_from_this(), placeholders::_1, placeholders::_2));
}
