#pragma once
#include "JSONItem.h"

using namespace std;

class JSONInteger: public JSONItem
{
private:
    int64_t m_data;

public:
    JSONInteger(int64_t data) : m_data(data) {};

    virtual int64_t getInteger() const override 
    { 
        return m_data;
    };

    virtual string getJSONString(int indent = 0) const override
    {
        return to_string(m_data);
    }
};

