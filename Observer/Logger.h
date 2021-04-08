#pragma once
#include <chrono>
#include <string>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>

using namespace std;

class LogWriter;

class Logger
{
public:
	enum class level { debug = 0, info = 1, warning = 2, error = 3 };

private:
	vector<pair<level, unique_ptr<LogWriter>>> m_writers;
	mutex m_mutex;

	Logger();
	~Logger();
	Logger(Logger const&) = delete;
	Logger& operator=(Logger const&) = delete;

	void flush();
	void out(level logLevel, const string& msg);

public:
	static Logger& getInstance()
	{
		static Logger instance;
		return instance;
	}

	static void d(const string& msg);
	static void e(const string& msg)
	{
		Logger::getInstance().out(level::error, msg);
	}
	static void i(const string& msg)
	{
		Logger::getInstance().out(level::info, msg);
	}
	static void w(const string& msg)
	{
		Logger::getInstance().out(level::warning, msg);
	}

	void addFile(level logLevel, const string& fileName);
	void setConsoleLogLevel(level logLevel);
};
