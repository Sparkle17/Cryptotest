#pragma once
#include <string>

#include "JSONItem.h"

class JSONString: public JSONItem
{
private:
    string m_data;

public:
    JSONString(const string& data) : m_data(data) {};

    virtual string getString() const override
    {
        return m_data;
    };

    virtual string getJSONString(int indent = 0) const override
    {
        return '\"' + m_data + '\"';
    }
};

