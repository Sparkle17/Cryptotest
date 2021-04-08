#pragma once
#include "JSONItem.h"

using namespace std;

class JSONBoolean: public JSONItem
{
private:
    bool m_data;

public:
    JSONBoolean(bool data) : m_data(data) {};

    virtual bool getBoolean() const override
    {
        return m_data;
    };

    virtual string getJSONString(int indent = 0) const override
    {
        return m_data ? "TRUE" : "FALSE";
    }
};

