#ifndef DB_UTIL_HPP_
#define DB_UTIL_HPP_

#include <string>
#include "RepresentativeInfo.pb.h"

namespace WDb
{
	const unsigned VOTE_KEY_BUFFER_MAX_SIZE = 50;
	bool convertDateAndVoteNumToVoteKey(const std::string & data, const unsigned num, std::string & key);
	const char * convertPartyEnumToString(WDbType::PoliticalPartyInfo::PoliticalParties party);
}

#endif
