#include "JSONArray.h"
#include <sstream>

#include "json.h"

string JSONArray::getJSONString(int indent) const
{
	stringstream ss;
	string sIndent(indent * json::baseIndent, ' ');
	ss << '[';

	indent++;
	string sItemIndent(indent * json::baseIndent, ' ');
	bool first = true;
	for (const auto& item : m_items) {
		if (!first) ss << ',';
		first = false;
		ss << endl << sItemIndent;
		ss << item.getJSONString(indent);
	}

	ss << endl << sIndent << ']';
	return ss.str();
}
