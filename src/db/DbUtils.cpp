#include "DbUtils.hpp"

#include <stdio.h>

using namespace WDb;

bool WDb::convertDateAndVoteNumToVoteKey(const std::string & date, const unsigned num, std::string & key)
{
	char buffer[VOTE_KEY_BUFFER_MAX_SIZE];
	int charsWriten = sprintf(buffer, "%s:%d", date.c_str(), num);
	if (charsWriten < 0) {
		return false;
	}

	key.assign(buffer, charsWriten);
	return true;
}

const char * WDb::convertPartyEnumToString(WDbType::PoliticalPartyInfo::PoliticalParties party)
{
	switch (party) {
		case WDbType::PoliticalPartyInfo::KD:
			return "Kristillis demokraatit";
			break;
		case WDbType::PoliticalPartyInfo::VIHREAT:
			return "Vihreat";
			break;
		case WDbType::PoliticalPartyInfo::KOKOOMUS:
			return "Kokoomus";
			break;
		case WDbType::PoliticalPartyInfo::VASEMMISTO:
			return "Vasemmisto";
			break;
		case WDbType::PoliticalPartyInfo::KESKUSTA:
			return "Keskusta";
			break;
		case WDbType::PoliticalPartyInfo::RKP:
			return "RKP";
			break;
		case WDbType::PoliticalPartyInfo::PS:
			return "PS";
			break;
		case WDbType::PoliticalPartyInfo::SDP:
			return "SDP";
			break;
		default:
			return "Unknown";
			break;
	}

	return "";
}
