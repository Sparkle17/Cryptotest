#pragma once
#include "JSONItem.h"
#include <memory>

class JSONData: public IJSONItem
{
private:
	unique_ptr<JSONItem> m_data;

public:
	JSONData() {};
	JSONData(unique_ptr<JSONItem>&& data) : m_data(move(data)) {};

	virtual void add(JSONData&& obj) override { m_data->add(move(obj)); }

	virtual JSONData& operator[](const string& name) override { return m_data->operator[](name); }
	virtual JSONData& operator[](size_t index) override { return m_data->operator[](index); }

	virtual bool getBoolean() const override { return m_data ? m_data->getBoolean() : false; }
	virtual int64_t getInteger() const override { return m_data ? m_data->getInteger() : 0; }
	virtual string getString() const override {	return m_data ? m_data->getString() : ""; }

	virtual string getJSONString(int indent = 0) const override { return m_data ? m_data->getJSONString(indent) : ""; }

	virtual bool isNull() const override { return m_data ? m_data->isNull() : true; }
	virtual size_t size() const override { return m_data ? m_data->size() : 0; }
};
