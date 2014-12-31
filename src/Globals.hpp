#ifndef OC_GLOBALS
#define OC_GLOBALS

#include <string>

namespace Wt
{
	class WString;
}

namespace WDbType
{
	class UserInfo;
	class VoteInfo;
	class RepresentativeInfo;
	class ListOfVotesForMonth;
	class PoliticalSeasonSeating;
	class PoliticalSeasonsList;
	class PoliticalSeason;
	class UserVoteList;
	class NewsIndex;
	class NewsItem;
	class PoliticalPartyNames;
	class VoteRecord;
	class Dictionary;
	class Collections;
	class CollectionOfUpdateSources;
	class UpdateSource;
	class UpdateItem;
}

namespace WGlobals
{
	typedef WDbType::UserInfo UserInformation;
	typedef WDbType::VoteInfo VoteInformation;
	typedef WDbType::ListOfVotesForMonth ListOfVotesForMonth;
	typedef WDbType::RepresentativeInfo RepresentativeInfo;
	typedef WDbType::PoliticalSeasonSeating PoliticalSeasonSeating;
	typedef WDbType::PoliticalSeasonsList PoliticalSeasonsList;
	typedef WDbType::PoliticalSeason PoliticalSeason;
	typedef WDbType::UserVoteList UserVoteList;
	typedef WDbType::NewsIndex NewsIndex;
	typedef WDbType::NewsItem NewsItem;
	typedef WDbType::PoliticalPartyNames PoliticalPartyNames;
	typedef WDbType::VoteRecord VoteRecord;
	typedef WDbType::Dictionary VoteStatistics;
	typedef WDbType::Collections Collections;
	typedef WDbType::CollectionOfUpdateSources CollectionOfUpdateSources;
	typedef WDbType::UpdateSource UpdateSource;
	typedef WDbType::UpdateItem UpdateItem;

	const unsigned USERNAME_MAX_LENGTH = 30;
	const unsigned USERVOTELIST_NAME_MAX_LENGTH = 80;

	static const unsigned COOKIE_TIME_TO_LIVE_SECONDS = 60*60*24*7;
	static const char * const COOKIE_APP_NAME = "WCookie";
	static const char * const COOKIE_SERVICE_NAME = "W";

	bool convertStrTimestampToUInt(const std::string & str, unsigned long & value);
	bool convertStrToUInt(const std::string & str, unsigned & value);
	bool convertStringToTimet(const char * source, const char * format, time_t & destination);
	bool convertUIntToStr(const unsigned value, std::string & target);
	bool convertIntToStr(const int value, std::string & target);
	Wt::WString convertUnsignedMonthToStr(const unsigned month);
}

#endif
