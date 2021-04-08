#include "BookLoaderBinance.h"
#include <iostream>
#include <mutex>
#include <string>

#include <boost/algorithm/string.hpp>

#include "../../Logger.h"
#include "../../Network.h"

namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

BookLoaderBinance::BookLoaderBinance(const PairInfo& info)
  : BookLoader(info.base, info.quoted)
//    m_websocket(Network::getContext(), Network::getSSLContext()),
//    m_apisocket(Network::getContext(), Network::getSSLContext()),
//    m_updateDone(false)
{
    m_minLotSize = info.minLotSize;
    m_notion = info.notion;

    m_lastOrderbookUpdateId = 0;
}

/*void BookLoaderBinance::bookConnectHandler(const boost::system::error_code& err)
{
    if (!err)
        m_apisocket.async_handshake(ssl::stream_base::client, bind(&BookLoaderBinance::bookHandshakeHandler, shared_from_base<BookLoaderBinance>(), placeholders::_1));
}

void BookLoaderBinance::bookHandshakeHandler(const boost::system::error_code& err)
{
    if (!err)
        http::async_write(m_apisocket, m_apiheader, bind(&BookLoaderBinance::bookWriteHandler, shared_from_base<BookLoaderBinance>(), placeholders::_1, placeholders::_2));
}

void BookLoaderBinance::bookReadHandler(const boost::system::error_code& err, size_t bytes_transferred)
{
    if (!err) {
        parseBook(m_apidata.body());
        onUpdate();
    }
    m_apisocket.async_shutdown(bind(&BookLoaderBinance::closeHandler, shared_from_base<BookLoaderBinance>(), placeholders::_1));
}

void BookLoaderBinance::bookWriteHandler(const boost::system::error_code& err, size_t bytes_transferred)
{
    if (!err)
        http::async_read(m_apisocket, m_apibuffer, m_apidata, bind(&BookLoaderBinance::bookReadHandler, shared_from_base<BookLoaderBinance>(), placeholders::_1, placeholders::_2));
}

void BookLoaderBinance::closeHandler(const boost::system::error_code& err)
{
}*/

void BookLoaderBinance::connect()
{
    BookLoader::connect();
    //m_ticker = make_shared<BinanceWebsocket>("/ws/" + boost::to_lower_copy(m_pair) + "@bookTicker", bind(&BookLoaderBinance::parseTicker, shared_from_base<BookLoaderBinance>(), placeholders::_1, placeholders::_2));
    //m_ticker->connect();
    //net::async_connect(get_lowest_layer(m_websocket), Network::binance_ws().resolved, bind(&BookLoaderBinance::streamConnectHandler, shared_from_base<BookLoaderBinance>(), placeholders::_1, placeholders::_2));
}

void BookLoaderBinance::disconnect()
{
    //m_websocket.async_close(websocket::close_code::normal, bind(&BookLoaderBinance::closeHandler, shared_from_base<BookLoaderBinance>(), placeholders::_1));
    //m_ticker->disconnect();
    BookLoader::disconnect();
}

/*void BookLoaderBinance::readNext()
{
    m_websocket.async_read(m_buffer, bind(&BookLoaderBinance::streamReadHandler, shared_from_base<BookLoaderBinance>(), placeholders::_1, placeholders::_2));
}

void BookLoaderBinance::parseBook(const string& data)
{
    try {
        JSONLoader json(data);
        json.skip({ "{", "\"lastUpdateId\":" });

        lock_guard<shared_mutex> lock(m_mutex);
        m_lastUpdateId = json.getNumber();
        json.skip({ ",", "\"bids\":", "[" });
        parseOrders(json, m_buys);
        json.skip({",", "\"asks\":", "[" });
        parseOrders(json, m_sells);

        for (const auto& data : m_cache)
            parseUpdate(data);
        m_cache.clear();
        m_updateDone = true;
    }
    catch (exception const&) {
        throw(new exception("parseBook failed"));
    }
}

void BookLoaderBinance::parseOrders(JSONLoader& json, map<PreciseNumber, PreciseNumber>& orders)
{
    char next = json.getSymbol();
    if (next == '[')
        while (true) {
            auto price = json.getPreciseNumber();
            json.skip(",");
            auto qty = json.getPreciseNumber();
            if (qty > 0) orders[price] = qty;
            else orders.erase(price);

            json.skip("]");
            next = json.getSymbol();
            if (next != ',') break;
            json.skip("[");
        }
}

// calls of this function are already locked
void BookLoaderBinance::parseUpdate(const string& data)
{
    try {
        JSONLoader json(data);
        json.skipTo("\"u\":");
        auto u = json.getNumber();
        if (u > m_lastUpdateId) {
            json.skipTo("\"b\":");
            json.skip("[");
            parseOrders(json, m_buys);
            json.skip({ ",", "\"a\":", "[" });
            parseOrders(json, m_sells);
        }
    }
    catch (exception const&) {
        throw(new exception("parseUpdate failed"));
    }
}*/

void BookLoaderBinance::parseTicker(const string& data, bool success)
{
    BinanceTrader::m_bookHits++;
    try {
        JSONLoader json(data);
        json.skip("{\"u\":");
        uint64_t id = json.getNumber();
        if (id > m_lastOrderbookUpdateId) {
            m_lastOrderbookUpdateId = id;
            json.skip(",\"s\":\"");
            json.skipTo('\"');
            json.skip(",\"b\":");
            m_bestBid = json.getPreciseNumber();
            json.skip(",\"B\":");
            m_bestBidQuantity = json.getPreciseNumber();
            json.skip(",\"a\":");
            m_bestAsk = json.getPreciseNumber();
            json.skip(",\"A\":");
            m_bestAskQuantity = json.getPreciseNumber();
            onUpdate();
        }
    }
    catch (exception const&) {
        Logger::e("parsing failed: " + data);
    }
}

/*void BookLoaderBinance::streamConnectHandler(const boost::system::error_code& err, tcp::resolver::results_type::endpoint_type ep)
{
    if (!err) {
        auto addr = Network::binance_ws();
        if (!SSL_set_tlsext_host_name(m_websocket.next_layer().native_handle(), addr.host.c_str()))
            throw beast::system_error(
                beast::error_code(
                    static_cast<int>(::ERR_get_error()),
                    net::error::get_ssl_category()),
                "Failed to set SNI Hostname");
        string phost = addr.host + ':' + std::to_string(ep.port());
        m_websocket.next_layer().async_handshake(ssl::stream_base::client, bind(&BookLoaderBinance::streamHandshakeHandler, shared_from_base<BookLoaderBinance>(), placeholders::_1, phost));
    }
}

void BookLoaderBinance::streamHandshakeHandler(const boost::system::error_code& err, string wshost)
{
    if (!err)
        m_websocket.async_handshake(wshost, "/ws/" + boost::to_lower_copy(m_pair) + "@depth@100ms", bind(&BookLoaderBinance::streamHandshakeWsHandler, shared_from_base<BookLoaderBinance>(), placeholders::_1));
}

void BookLoaderBinance::streamHandshakeWsHandler(const boost::system::error_code& err)
{
    readNext();

    auto apiaddr = Network::binance_api();
    m_apiheader.version(11);
    m_apiheader.method(http::verb::get);
    m_apiheader.target("/api/v3/depth?symbol=" + m_pair + "&limit=100");
    m_apiheader.set(http::field::host, apiaddr.host);
    m_apiheader.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    net::async_connect(get_lowest_layer(m_apisocket), apiaddr.resolved, bind(&BookLoaderBinance::bookConnectHandler, shared_from_base<BookLoaderBinance>(), placeholders::_1));
}

void BookLoaderBinance::streamReadHandler(const boost::system::error_code& err, size_t bytes_transferred)
{
    if (!err && !isDisconnected()) {
        unique_lock<shared_mutex> lock(m_mutex);
        string data = beast::buffers_to_string(m_buffer.data());
        if (m_updateDone)
            parseUpdate(data);
        else
            m_cache.push_back(move(data));
        lock.unlock();

        if (m_updateDone)
            onUpdate();

        m_buffer.clear();
        readNext();
    }
}*/

void BookLoaderBinance::updateTicker(uint64_t id, PreciseNumber bestBid, PreciseNumber bestBidQuantity, PreciseNumber bestAsk, PreciseNumber bestAskQuantity)
{
    if (id > m_lastOrderbookUpdateId) {
        m_lastOrderbookUpdateId = id;
        m_bestBid = bestBid;
        m_bestBidQuantity = bestBidQuantity;
        m_bestAsk = bestAsk;
        m_bestAskQuantity = bestAskQuantity;
        onUpdate();
    }
}
