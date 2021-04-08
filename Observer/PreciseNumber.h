#pragma once
#include <immintrin.h>
#include <intrin.h>
#include <string>

using namespace std;

class PreciseNumber
{
private:
	static const int64_t fracPow = 100000000;
	static const int fracLen = 8;

	int64_t data;

public:
	PreciseNumber() : data(0) {};
	PreciseNumber(int64_t ndata) : data(ndata* fracPow) {};
	PreciseNumber(const string& str);
	static PreciseNumber fromPrecise(int64_t precised)
	{
		PreciseNumber result;
		result.data = precised;
		return result;
	}

	string to_string() const;
	void align(PreciseNumber minValue);

	bool operator <(const PreciseNumber& value) const
	{
		return data < value.data;
	}

	bool operator >(const PreciseNumber& value) const
	{
		return data > value.data;
	}

	bool operator <=(const PreciseNumber& value) const
	{
		return data <= value.data;
	}

	bool operator >=(const PreciseNumber& value) const
	{
		return data >= value.data;
	}

	bool operator ==(const PreciseNumber& value) const
	{
		return data == value.data;
	}

	constexpr PreciseNumber& operator +=(const PreciseNumber& value)
	{
		data += value.data;
		return *this;
	}

	constexpr PreciseNumber& operator -=(const PreciseNumber& value)
	{
		data -= value.data;
		return *this;
	}

	PreciseNumber& operator *=(const PreciseNumber& value)
	{
		int64_t high;
		int64_t rem;
		int64_t low = _mul128(data, value.data, &high);
		data = _div128(high, low, fracPow, &rem);
		return *this;
	}

	PreciseNumber& operator /=(const PreciseNumber& value)
	{
		int64_t high;
		int64_t rem;
		int64_t low = _mul128(data, fracPow, &high);
		data = _div128(high, low, value.data, &rem);
		return *this;
	}

	PreciseNumber operator /(const int32_t value)
	{
		int64_t rem;
		int64_t newdata = _div128(0, data, value, &rem);
		return fromPrecise(newdata);
	}
};

PreciseNumber operator +(const PreciseNumber& value1, const PreciseNumber& value2);
PreciseNumber operator -(const PreciseNumber& value1, const PreciseNumber& value2);
PreciseNumber operator *(const PreciseNumber& value1, const PreciseNumber& value2);
PreciseNumber operator /(const PreciseNumber& value1, const PreciseNumber& value2);
