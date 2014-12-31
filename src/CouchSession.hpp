#include "DatabaseSession.hpp"

#include <unistd.h>//required by couchbase
#include <libcouchbase/couchbase.h>

#include <vector>
#include <string>
#include <utility>

namespace W
{
	static const char * const DB_USERNAME_TAG = "UserInfo:";
	static const char * const DB_VOTES_LIST_TAG = "VoteList:";
	static const char * const DB_VOTE_TAG = "VoteInfo:";
	static const char * const DB_REPRESENTATIVE_TAG = "RepresentativeInfo:";
	static const char * const DB_SEASON_SEATING_TAG = "Seating:";
	static const char * const DB_POLITICAL_SEASON_LIST_TAG = "PoliticalSeasonsList";
	static const char * const DB_POLITICAL_SEASON_INFO_TAG = "PoliticalSeasonInfo:";
	static const char * const DB_USER_VOTE_LIST_TAG = "UserVoteList:";
	static const char * const DB_USER_REF = "UserRef:";
	static const char * const DB_NEWS_INDEX_TAG = "NewsIndex";
	static const char * const DB_NEWS_ITEM_TAG = "NewsItem:";
	static const char * const DB_PARTY_NAME_INFO_TAG = "PoliticalPartyNames";
	static const char * const DB_VOTE_RECORD_TAG = "VoteRecord:";
	static const char * const DB_VOTE_STATISTICS_TAG = "VoteStat:";
	static const char * const DB_COLLECTIONS_TAG = "Collections";
	static const char * const DB_COLLECTION_TAG = "Collection:";
	static const char * const DB_UPDATE_SOURCE_TAG = "UpdateSource:";
	static const char * const DB_UPDATE_ITEM_TAG = "UpdateItem:";

	static const char * const DEFAULT_FIELD_VALUE = "NotSet";

	namespace DbRequestType
	{
		enum DbRequestType
		{
			GET_USERS_PER_COUNTRY = 1,
			GET_USER_INFO = 2,
			GET_VOTES_LIST = 3,
			GET_VOTE = 4,
			GET_REPRESENTATIVE = 5,
			GET_SEASON_SEATING = 6,
			GET_POLITICAL_SEASONS_LIST = 7,
			GET_POLITICAL_SEASON = 8,
			GET_USER_VOTE_LIST = 9,
			GET_UNSTRUCTURED_DATA = 10,
			GET_USER_REF = 11,
			GET_NEWS_INDEX = 12,
			GET_NEWS_ITEM = 13,
			GET_PARTY_NAME_INFO = 14,
			GET_VOTE_RECORD = 15,
			GET_VOTE_STATISTICS = 16,
			GET_COLLECTIONS = 17,
			GET_COLLECTION = 18,
			GET_UPDATE_SOURCE = 19,
			GET_UPDATE_ITEM = 20
		};
	}

	static const unsigned DB_COOKIE_GET_USERS_PER_COUNTRY = 1;
	static const unsigned DB_COOKIE_GET_USER_INFO = 2;
	static const unsigned COUCHSESSION_KEY_BUFFER_SIZE = 512;
	static const unsigned COUCHSESSION_DATA_BUFFER_SIZE = 2*1024;

	static const unsigned DB_USERINFO_VERSION = 1;

	struct SessionInformation;

	class CouchSession : public DatabaseSession
	{
		public:
			CouchSession(void);
			virtual ~CouchSession(void);
			void connectToDatabase();
			bool createNewUser(const char * username, const size_t usernameLength);
			bool setUserData(WGlobals::UserInformation & user);
			bool validateUser(const WGlobals::UserInformation & user);
			bool userExists(const char * username, const size_t & usernameLength);
			const WGlobals::UserInformation & findUser(const char * username, const size_t & usernameLength);
			bool deleteUser(const WGlobals::UserInformation & user);
			const WGlobals::ListOfVotesForMonth & findVotesForMonth(const unsigned year, const unsigned month);
			const WGlobals::VoteInformation & findVote(const char * voteId, const size_t idLength);
			const WGlobals::PoliticalSeasonSeating & findSeatingForSeason(const unsigned seasonStartingYear);
			const WGlobals::RepresentativeInfo & findRepresentativeInfo(const unsigned representativeId);
			const WGlobals::PoliticalSeasonsList & findPoliticalSeasonsList(void);
			const WGlobals::PoliticalSeason & findPoliticalSeason(const unsigned seasonStartingYear);
			const WGlobals::UserVoteList & findUserVoteList(const char * username, const unsigned usernameLength, const unsigned listId);
			void setUserVoteList(const char * username, const unsigned usernameLength, const unsigned listId, const WGlobals::UserVoteList & userVoteList);
			void setUserReferece(const char * username, const unsigned usernameLength, const char * key, const unsigned keyLength);
			const WGlobals::UserInformation & findUserByReferece(const char * key, const unsigned keyLength);
			const WGlobals::NewsIndex & findNewsIndex(void);
			const WGlobals::NewsItem & findNewsItem(const char * key, const unsigned keyLength);
			const WGlobals::PoliticalPartyNames & findPoliticalPartyNameInfo(void);
			const WGlobals::VoteRecord & findVoteRecord(const char * date, const unsigned dateLength);
			const WGlobals::VoteStatistics & findVoteStatistics(const char * voteId, const unsigned idLength);
			const WGlobals::Collections & findCollections(void);
			const WGlobals::CollectionOfUpdateSources & findCollectionOfUpdateSources(const char * name, const unsigned nameLength);
			const WGlobals::UpdateSource & findUpdateSource(const char * name, const unsigned nameLength);
			const WGlobals::UpdateItem & findUpdateItem(const char * baseName, const unsigned baseNameLength, const unsigned newsNumber);
			bool setUnstructuredData(const char * key, const unsigned keyLength, const char * data, const unsigned dataLength, unsigned expirationLength = 0);
			bool findUnstructuredData(const char * key, const unsigned keyLength, std::string & data);
			bool clearUnstructuredData(const char * key, const unsigned keyLength);

			inline SessionInformation * getSessionInformation(void) {return this->sessionInformation;}

		private:
			char keyBuffer[COUCHSESSION_KEY_BUFFER_SIZE + 1];
			char dataBuffer[COUCHSESSION_DATA_BUFFER_SIZE + 1];
			SessionInformation * sessionInformation;

			template<class T>
			bool writeObjectToDb(const char * key, const unsigned keyLength, T & t, const uint64_t dataVersion = 0);
			template<class T>
			bool readObjectFromDb(const char * key, const unsigned keyLength, DbRequestType::DbRequestType requestType, T & t);

	};
}

