#include "PreciseNumber.h"
#include <array>

static const array<int64_t, 12> pows{ { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 10000000000, 100000000000, 1000000000000 } };

PreciseNumber::PreciseNumber(const string& str) {

	data = 0;
	size_t point = -1;
	for (size_t i = 0; i < str.length(); i++) {
		char c = str[i];
		if (c >= '0' && c <= '9')
			data = data * 10 + c - '0';
		else if (c == '.')
			point = i;
		else throw(new exception("PreciseNumber: invalid string"));
	}
	if (point == -1)
		data *= pows[8];
	else {
		size_t pow = str.length() - point - 1;
		if (pow <= 8)
			data *= pows[8 - pow];
		else
			throw(new exception("PreciseNumber: too long fraction part"));
	}
}

void PreciseNumber::align(PreciseNumber minValue)
{
	if (minValue.data > 0)
		data -= data % minValue.data;
}

string PreciseNumber::to_string() const
{
	auto locData = data;
	string result;
	if (locData < 0) {
		result = "-";
		locData = -locData;
	}

	int64_t intPart = locData / fracPow;
	int32_t fracPart = locData % fracPow;

	result += std::to_string(intPart);
	if (fracPart > 0) {
		int len = fracLen;
		while (fracPart % 10 == 0) {
			fracPart /= 10;
			len--;
		}
		string frac = std::to_string(fracPart);
		while (frac.length() < len)
			frac = '0' + frac;
		result += "." + frac;
	}
	return result;
}

PreciseNumber operator +(const PreciseNumber& value1, const PreciseNumber& value2)
{
	PreciseNumber result(value1);
	result += value2;
	return result;
}

PreciseNumber operator -(const PreciseNumber& value1, const PreciseNumber& value2)
{
	PreciseNumber result(value1);
	result -= value2;
	return result;
}

PreciseNumber operator *(const PreciseNumber& value1, const PreciseNumber& value2)
{
	PreciseNumber result(value1);
	result *= value2;
	return result;
}

PreciseNumber operator /(const PreciseNumber& value1, const PreciseNumber& value2)
{
	PreciseNumber result(value1);
	result /= value2;
	return result;
}
