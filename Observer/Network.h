#pragma once
#include <memory>
#include <string>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/thread/thread.hpp>

using namespace std;

typedef boost::beast::http::response<boost::beast::http::string_body> HttpResponse;
typedef function<void(bool)> ConnectHandler;
typedef function<void(bool)> DisconnectHandler;
typedef function<void(string, bool, HttpResponse)> HttpRequestHandler;
typedef function<void(string, bool)> WebsocketRequestHandler;

struct NetAddress
{
public:
	const string host;
	const string port;
	boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp> resolved;

	NetAddress(const string& _host, const string& _port)
		: host(_host), port(_port)
	{
	}

	void resolve(boost::asio::ip::tcp::resolver& resolver)
	{
		resolved = resolver.resolve(host, port);
	}
};

class Network
{
private:
	boost::asio::io_context m_context;
	boost::asio::ssl::context m_sslcontext;
	boost::asio::ip::tcp::resolver m_resolver;

	boost::thread_group m_threadpool;
	boost::asio::executor_work_guard<decltype(m_context.get_executor())> m_workguard;

	NetAddress addr_binance_ws;
	NetAddress addr_binance_api;

	Network() 
	  :	m_sslcontext(boost::asio::ssl::context::method::sslv23),
		m_resolver(m_context),

		m_workguard(m_context.get_executor()),

		addr_binance_ws("stream.binance.com", "9443"),
		addr_binance_api("api2.binance.com", "443")
	{
		//m_sslcontext.set_verify_mode(boost::asio::ssl::verify_peer);

		addr_binance_ws.resolve(m_resolver);
		addr_binance_api.resolve(m_resolver);

		auto maxThreads = thread::hardware_concurrency();
		for (unsigned int i = 0; i < maxThreads; i++)
			m_threadpool.create_thread(boost::bind(&boost::asio::io_context::run, &m_context));
	}

	~Network()
	{
	}

	Network(Network const&) = delete;
	Network& operator=(Network const&) = delete;

	void do_disconnect() 
	{
		m_workguard.reset();
		m_threadpool.join_all();
	}

public:
	static const NetAddress& binance_api()
	{
		return getInstance().addr_binance_api;
	}

	static const NetAddress& binance_ws()
	{
		return getInstance().addr_binance_ws;
	}

	static void disconnect() 
	{
		getInstance().do_disconnect();
	}

	static Network& getInstance() 
	{
		static Network instance;
		return instance;
	}

	static boost::asio::io_context& getContext()
	{
		return getInstance().m_context;
	}

	static boost::asio::ssl::context& getSSLContext()
	{
		return getInstance().m_sslcontext;
	}
};