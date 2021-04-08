#include "json.h"
#include <fstream>
#include <sstream>

JSONData json::load(const string& fileName)
{
	ifstream fs;
	fs.open(fileName, ios_base::in);
	stringstream ss;
	ss << fs.rdbuf();
	JSONLoader json(ss.str());
	auto result = json.parse();
	fs.close();
	return result;
}

void json::save(const string& fileName, const JSONData data)
{
	ofstream fs;
	fs.open(fileName, ios_base::out | ios_base::trunc);
	fs << data.getJSONString();
	fs.close();
}
