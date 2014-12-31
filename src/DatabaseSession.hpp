#ifndef DATABASE_SESSION_HPP_
#define DATABASE_SESSION_HPP_

#include "Globals.hpp"

#include <stddef.h>

namespace W
{
	static const unsigned NUMBER_OF_DB_WRITE_RETIRES = 5;

	class DatabaseSession
	{
		public:
			DatabaseSession() {};
			virtual ~DatabaseSession(void) {};
			virtual void connectToDatabase() = 0;
			virtual bool createNewUser(const char * username, const size_t usernameLength) = 0;
			virtual bool setUserData(WGlobals::UserInformation & user) = 0;					//Not const for updates
			virtual bool validateUser(const WGlobals::UserInformation & user) = 0;
			virtual bool userExists(const char * username, const size_t & usernameLength) = 0;
			virtual void setUserReferece(const char * username, const unsigned usernameLength, const char * key, const unsigned keyLength) = 0;
			virtual const WGlobals::UserInformation & findUser(const char * username, const size_t & usernameLength) = 0;
			virtual bool deleteUser(const WGlobals::UserInformation & user) = 0;
			virtual const WGlobals::ListOfVotesForMonth & findVotesForMonth(const unsigned year, const unsigned month) = 0;
			virtual const WGlobals::VoteInformation & findVote(const char * voteId, const size_t idLength) = 0;
			virtual const WGlobals::PoliticalSeasonSeating & findSeatingForSeason(const unsigned seasonStartingYear) = 0;
			virtual const WGlobals::RepresentativeInfo & findRepresentativeInfo(const unsigned representativeId) = 0;
			virtual const WGlobals::PoliticalSeasonsList & findPoliticalSeasonsList(void) = 0;
			virtual const WGlobals::PoliticalSeason & findPoliticalSeason(const unsigned seasonStartingYear) = 0;
			virtual const WGlobals::UserVoteList & findUserVoteList(const char * username, const unsigned usernameLength, const unsigned listId) = 0;
			virtual const WGlobals::UserInformation & findUserByReferece(const char * key, const unsigned keyLength) = 0;
			virtual const WGlobals::NewsIndex & findNewsIndex(void) = 0;
			virtual const WGlobals::NewsItem & findNewsItem(const char * key, const unsigned keyLength) = 0;
			virtual const WGlobals::PoliticalPartyNames & findPoliticalPartyNameInfo(void) = 0;
			virtual const WGlobals::VoteRecord & findVoteRecord(const char * date, const unsigned dateLength) = 0;
			virtual const WGlobals::VoteStatistics & findVoteStatistics(const char * voteId, const unsigned idLength) = 0;
			virtual const WGlobals::Collections & findCollections(void) = 0;
			virtual const WGlobals::CollectionOfUpdateSources & findCollectionOfUpdateSources(const char * name, const unsigned nameLength) = 0;
			virtual const WGlobals::UpdateSource & findUpdateSource(const char * name, const unsigned nameLength) = 0;
			virtual const WGlobals::UpdateItem & findUpdateItem(const char * baseName, const unsigned baseNameLength, const unsigned newsNumber) = 0;
			virtual void setUserVoteList(const char * username, const unsigned usernameLength, const unsigned listId, const WGlobals::UserVoteList & userVoteList) = 0;
			virtual bool setUnstructuredData(const char * key, const unsigned keyLength, const char * data, const unsigned dataLength, unsigned expirationLength = 0) = 0;
			virtual bool findUnstructuredData(const char * key, const unsigned keyLength, std::string & data) = 0;
			virtual bool clearUnstructuredData(const char * key, const unsigned keyLength) = 0;

		protected:

		private:

	};
}
#endif
