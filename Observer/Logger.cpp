#include "Logger.h"

#include "boost/date_time/posix_time/posix_time.hpp"

namespace pt = boost::posix_time;

class LogWriter
{
public:
	virtual ~LogWriter() {}
	virtual void flush() {}
	virtual void print(const string& message) = 0;
};

class ConsoleWriter: public LogWriter
{
public:
	virtual void print(const string& message) override
	{
		cout << message;
	}
};

class FileWriter : public LogWriter
{
private:
	ofstream m_file;

public:
	FileWriter(const string& fileName)
		: m_file(fileName, ios_base::app) {}
	virtual ~FileWriter() 
	{
		m_file.close();
	}

	virtual void flush() override
	{
		m_file.flush();
	}
	virtual void print(const string& message) override
	{
		m_file << message;
	}
};

Logger::Logger()
{
	m_writers.push_back({ level::info, make_unique<ConsoleWriter>() });
}

Logger::~Logger()
{
}

void Logger::addFile(level logLevel, const string& fileName)
{
	lock_guard<mutex> lock(m_mutex);
	m_writers.push_back({ logLevel, make_unique<FileWriter>(fileName) });
}

void Logger::d(const string& msg)
{
	Logger::getInstance().out(level::debug, msg);
	Logger::getInstance().flush();
}

void Logger::flush()
{
	for (auto& writer : m_writers)
		writer.second->flush();
}

void Logger::out(level logLevel, const string& msg)
{
	lock_guard<mutex> lock(m_mutex);
	pt::ptime now = pt::microsec_clock::local_time();
	stringstream ss;
	ss << now << ": " << msg << endl;
	for (auto& p : m_writers)
		if (p.first <= logLevel)
			p.second->print(ss.str());
}

void Logger::setConsoleLogLevel(level logLevel)
{
	lock_guard<mutex> lock(m_mutex);
	m_writers[0].first = logLevel;
}
