#include "Globals.hpp"

#include <Wt/WString>

#include <cstdio>
#include <string>
#include <ctime>
#include <cstring>
#include <stdexcept>

using namespace WGlobals;

bool WGlobals::convertStrTimestampToUInt(const std::string & str, unsigned long & value)
{
	return (sscanf(str.c_str(), "%lu", &value) == 1) ? true : false;
}

bool WGlobals::convertStrToUInt(const std::string & str, unsigned & value)
{
	return (sscanf(str.c_str(), "%u", &value) == 1) ? true : false;
}

bool WGlobals::convertStringToTimet(const char * source, const char * format, time_t & destination)
{
	struct tm time;
	memset(&time, 0, sizeof(time));
	if (strptime(source, format, &time) == 0) {
		return false;
	}
	destination = std::mktime(&time);
	return true;
}

bool WGlobals::convertUIntToStr(const unsigned value, std::string & target)
{
	static const unsigned sizeOfBuffer = 10;
	char buffer[sizeOfBuffer];
	if ((snprintf(buffer, sizeOfBuffer, "%u", value)) == 0) {
		return false;
	}
	target.append(buffer);
	return true;
}

bool WGlobals::convertIntToStr(const int value, std::string & target)
{
	static const unsigned sizeOfBuffer = 10;
	char buffer[sizeOfBuffer];
	if ((snprintf(buffer, sizeOfBuffer, "%d", value)) == 0) {
		return false;
	}
	target.append(buffer);
	return true;
}

Wt::WString WGlobals::convertUnsignedMonthToStr(const unsigned month)
{
	switch (month) {
		case 1:
			return Wt::WString::tr("month.jan");
		case 2:
			return Wt::WString::tr("month.feb");
		case 3:
			return Wt::WString::tr("month.mar");
		case 4:
			return Wt::WString::tr("month.apr");
		case 5:
			return Wt::WString::tr("month.may");
		case 6:
			return Wt::WString::tr("month.jun");
		case 7:
			return Wt::WString::tr("month.jul");
		case 8:
			return Wt::WString::tr("month.aug");
		case 9:
			return Wt::WString::tr("month.sep");
		case 10:
			return Wt::WString::tr("month.oct");
		case 11:
			return Wt::WString::tr("month.nov");
		case 12:
			return Wt::WString::tr("month.dec");
		default:
			throw std::invalid_argument ("convertUnsignedMonthToStr called with invalid month");
	}
}
