#pragma once
#include <memory>

#include "JSONArray.h"
#include "JSONBoolean.h"
#include "JSONData.h"
#include "JSONInteger.h"
#include "JSONLoader.h"
#include "JSONNull.h"
#include "JSONObject.h"
#include "JSONString.h"

using namespace std;
 
class json 
{
public:
	static const int baseIndent = 4;

	static JSONData createArray()
	{
		return JSONData(make_unique<JSONArray>());
	}
		
	static JSONData createBoolean(bool data)
	{
		return JSONData(make_unique<JSONBoolean>(data));
	}

	static JSONData createInteger(int64_t data)
	{
		return JSONData(make_unique<JSONInteger>(data));
	}

	static JSONData createNull()
	{
		return JSONData(make_unique<JSONNull>());
	}

	static JSONData createObject()
	{
		return JSONData(make_unique<JSONObject>());
	}

	static JSONData createString(const string& data)
	{
		return JSONData(make_unique<JSONString>(data));
	}

	static JSONData load(const string& fileName);
	static void save(const string& fileName, const JSONData data);
};