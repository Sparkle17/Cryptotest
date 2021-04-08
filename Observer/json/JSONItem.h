#pragma once
#include <stdexcept>
#include <string>

using namespace std;

class JSONData;

class IJSONItem
{
public:
	virtual void add(JSONData&& obj) = 0;

	virtual JSONData& operator[](const string& name) = 0;
	virtual JSONData& operator[](size_t index) = 0;
	
	virtual bool getBoolean() const = 0;
	virtual int64_t getInteger() const = 0;
	virtual string getString() const = 0;

	virtual string getJSONString(int indent = 0) const = 0;

	virtual bool isNull() const = 0;
	virtual size_t size() const = 0;
};

class JSONItem: public IJSONItem
{
private:
	static const string strInvalidFormat;

public:
	virtual ~JSONItem() = default;

	virtual void add(JSONData&& obj) override 
	{
		throw runtime_error(strInvalidFormat);
	};

	virtual JSONData& operator[](const string& name) override
	{
		throw runtime_error(strInvalidFormat);
	}
	virtual JSONData& operator[](size_t index) override 
	{
		throw runtime_error(strInvalidFormat);
	}

	virtual bool getBoolean() const override
	{
		throw runtime_error(strInvalidFormat);
	}
	virtual int64_t getInteger() const override
	{ 
		throw runtime_error(strInvalidFormat);
	}
	virtual string getString() const override
	{
		throw runtime_error(strInvalidFormat);
	}

	virtual string getJSONString(int indent = 0) const override
	{
		return "";
	}

	virtual bool isNull() const override
	{
		return false;
	}
	virtual size_t size() const override
	{
		return 0;
	}
};

