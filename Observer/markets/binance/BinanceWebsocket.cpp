#include "BinanceWebsocket.h"

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include "../../Logger.h"

namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

BinanceWebsocket::BinanceWebsocket(const string& address, WebsocketRequestHandler handler)
	: m_handler(handler),
	m_address(address),
    m_websocket(Network::getContext(), Network::getSSLContext())
{
}

void BinanceWebsocket::connect(ConnectHandler handler)
{
	m_connectHandler = handler;
	net::async_connect(get_lowest_layer(m_websocket), Network::binance_ws().resolved, bind(&BinanceWebsocket::connectHandler, shared_from_this(), placeholders::_1, placeholders::_2));
}

void BinanceWebsocket::connectHandler(const boost::system::error_code& err, tcp::resolver::results_type::endpoint_type ep)
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
        m_websocket.next_layer().async_handshake(ssl::stream_base::client, bind(&BinanceWebsocket::handshakeHandler, shared_from_this(), placeholders::_1, phost));
    }
    else
        Logger::e("websocker connect error (" + m_address + ")");
}

void BinanceWebsocket::closeHandler(const boost::system::error_code& err, DisconnectHandler handler)
{
    if (handler) handler(!err);
}

void BinanceWebsocket::disconnect(DisconnectHandler handler)
{
    bool b = true;
    if (m_connected.compare_exchange_strong(b, false)) {
        m_connected = false;
        m_websocket.async_close(websocket::close_code::normal, bind(&BinanceWebsocket::closeHandler, shared_from_this(), placeholders::_1, handler));
    }
    else
        if (handler) handler(true);
}

void BinanceWebsocket::handshakeHandler(const boost::system::error_code& err, string wshost)
{
    if (!err)
        m_websocket.async_handshake(wshost, m_address, bind(&BinanceWebsocket::handshakeWsHandler, shared_from_this(), placeholders::_1));
    else
        Logger::e("websocker ssl handshake error (" + m_address + ")");
}

void BinanceWebsocket::handshakeWsHandler(const boost::system::error_code& err)
{
    if (!err) {
        m_connected = true;
        if (m_connectHandler)
            m_connectHandler(!err);
        readNext();
    }
    else
        Logger::e("websocker handshake error (" + m_address + ")");
}

void BinanceWebsocket::readHandler(const boost::system::error_code& err, size_t bytes_transferred)
{
    if (isConnected()) {
        if (!err) {
            if (m_handler)
                m_handler(beast::buffers_to_string(m_buffer.data()), !err);
            m_buffer.clear();
            readNext();
        }
        else {
            Logger::e("websocker read error (" + m_address + ")");
        }
    }
}

void BinanceWebsocket::readNext()
{
    m_websocket.async_read(m_buffer, bind(&BinanceWebsocket::readHandler, shared_from_this(), placeholders::_1, placeholders::_2));
}
