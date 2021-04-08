#pragma once
#include <map>
#include <memory>

#include "JSONData.h"
#include "JSONItem.h"

using namespace std;

class JSONObject: public JSONItem
{
private:
    map<string, JSONData> m_items;

public:
    virtual JSONData& operator[](const string& name) override
    {
        return m_items[name];
    }

    virtual string getJSONString(int indent = 0) const override;
};

