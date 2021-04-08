#pragma once
#include <vector>

#include "JSONItem.h"

using namespace std;

class JSONArray: public JSONItem
{
private:
	vector<JSONData> m_items;

public:
    virtual void add(JSONData&& obj) override
    {
        m_items.push_back(move(obj));
    }

    virtual JSONData& operator[](size_t index) override
    {
        return m_items[index];
    }

    virtual string getJSONString(int indent = 0) const override;

    virtual size_t size() const override
    {
        return m_items.size();
    }
};

