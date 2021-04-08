#pragma once
#include <memory>
#include <string>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include "..\..\Network.h"

using namespace std;

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = boost::asio::ip::tcp;

class BinanceWebsocket: public enable_shared_from_this<BinanceWebsocket>
{
	websocket::stream<beast::ssl_stream<tcp::socket>> m_websocket;
	beast::flat_buffer m_buffer;

	WebsocketRequestHandler m_handler;
	ConnectHandler m_connectHandler;
	string m_address;
	atomic<bool> m_connected;

	void connectHandler(const boost::system::error_code& err, tcp::resolver::results_type::endpoint_type ep);
	void closeHandler(const boost::system::error_code& err, DisconnectHandler handler);
	void handshakeHandler(const boost::system::error_code& err, string wshost);
	void handshakeWsHandler(const boost::system::error_code& err);
	void readHandler(const boost::system::error_code& err, size_t bytes_transferred);
	void readNext();

public:
	BinanceWebsocket(const string& address, WebsocketRequestHandler handler);

	void connect(ConnectHandler handler = 0);
	void disconnect(DisconnectHandler handler = 0);
	bool isConnected()
	{
		return m_connected;
	}
};

