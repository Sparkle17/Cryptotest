#pragma once
#include <string>

#include "JSONData.h"
#include "JSONObject.h"

#include "..\PreciseNumber.h"

using namespace std;

class JSONLoader
{
private:
    static const string strParsingError;

	const string data;
	size_t index;
	const size_t length;

    void skipDelim()
    {
        while (index < length) {
            char c = data[index];
            if (c > ' ') return;
            index++;
        }
    };

    JSONData parseArray();
    JSONData parseBoolean();
    JSONData parseInteger();
    JSONData parseItem();
    JSONData parseObject();
    JSONData parseString();

public:
	JSONLoader(const string& _data);
    JSONLoader(string&& _data);

    bool getBool();
    int64_t getNumber();
    char getSymbol();
    PreciseNumber getPreciseNumber();
    string getString();

    JSONData parse();

    void skip(const string& top);
    void skip(initializer_list<string> strs);
    void skipTo(const string& substr);
    void skipTo(char ch);
};

