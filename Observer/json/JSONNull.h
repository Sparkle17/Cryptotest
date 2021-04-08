#pragma once
#include "JSONItem.h"

class JSONNull: public JSONItem
{
public:
    virtual bool isNull() const override
    {
        return true;
    };

    virtual string getJSONString(int indent = 0) const override
    {
        return "NULL";
    }
};

