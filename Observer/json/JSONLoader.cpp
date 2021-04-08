#include "JSONLoader.h"
#include <algorithm>
#include <array>

#include "json.h"

const string JSONLoader::strParsingError = "Error parsing JSON data";

JSONLoader::JSONLoader(const string& _data)
  : data(_data),
	index(0),
	length(data.length())
{
}

JSONLoader::JSONLoader(string&& _data)
	: data(move(_data)),
	index(0),
	length(data.length())
{
}

bool JSONLoader::getBool() {
	skipDelim();
	auto start = index;
	while (index < length) {
		char c = data[index];
		if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z')) break;
		index++;
	}
	string str = data.substr(start, index - start);
	transform(str.begin(), str.end(), str.begin(), toupper);
	return str == "TRUE";
}

int64_t JSONLoader::getNumber()
{
	skipDelim();
	int64_t value = 0;
	bool negative = false;
	while (index < length) {
		char c = data[index];
		if (c >= '0' && c <= '9')
			value = value * 10 + c - '0';
		else if (c == '-')
			negative = true;
		else break;
		index++;
	}
	if (negative) value = -value;
	return value;
}

string JSONLoader::getString() {
	skip("\"");
	auto start = index;
	skipTo("\"");
	return data.substr(start, index - start - 1);
}

char JSONLoader::getSymbol()
{
	skipDelim();
	return data[index++];
}

PreciseNumber JSONLoader::getPreciseNumber()
{
	return PreciseNumber(getString());
}

JSONData JSONLoader::parse()
{
	index = 0;
	return parseItem();
}

JSONData JSONLoader::parseArray()
{
	char ch = getSymbol();
	if (ch != '[')
		throw(runtime_error(strParsingError));

	auto obj = json::createArray();
	ch = getSymbol();
	if (ch != ']') {
		index--;
		do {
			obj.add(parseItem());
			ch = getSymbol();
		} while (ch == ',');

		if (ch != ']')
			throw(runtime_error(strParsingError));
	}

	return obj;
}

JSONData JSONLoader::parseBoolean()
{
	return json::createBoolean(getBool());
}

JSONData JSONLoader::parseInteger()
{
	return json::createInteger(getNumber());
}

JSONData JSONLoader::parseItem()
{
	char ch = toupper(getSymbol());
	index--;

	if (ch == '-' || (ch >= '0' && ch <= '9'))
		return parseInteger();
	else if (ch == '\"')
		return parseString();
	else if (ch == '{')
		return parseObject();
	else if (ch == '[')
		return parseArray();
	else if (ch == 'T' || ch == 'F')
		return parseBoolean();
	else if (ch == 'N') {
		getBool();
		return json::createNull();
	}

	throw(runtime_error(strParsingError));
}

JSONData JSONLoader::parseObject()
{
	char ch = getSymbol();
	if (ch != '{')
		throw(runtime_error(strParsingError));

	auto obj = json::createObject();
	ch = getSymbol();
	if (ch != '}') {
		index--;
		do {
			string name = getString();
			skip(":");
			obj[name] = parseItem();
			ch = getSymbol();
		} while (ch == ',');

		if (ch != '}')
			throw(runtime_error(strParsingError));
	}

	return obj;
}

JSONData JSONLoader::parseString()
{
	return json::createString(getString());
}

void JSONLoader::skip(const string& top)
{
	skipDelim();
	auto toplen = top.length();
	if (toplen > length - index)
		throw(runtime_error("skip: not enough length"));
	for (size_t i = 0; i < toplen; i++)
		if (top[i] != data[index++])
			throw(runtime_error("skip: top doesn't match"));
}

void JSONLoader::skip(initializer_list<string> strs)
{
	for (auto arg : strs)
		skip(arg);
}

void JSONLoader::skipTo(const string& substr)
{
	auto idx = data.find(substr, index);
	if (idx != string::npos)
		index = idx + substr.length();
	else
		throw (runtime_error("skipTo: substring not found"));
}

void JSONLoader::skipTo(char ch)
{
	while (index < length && data[index] != ch)
		index++;
	if (index < length) index++;
}
