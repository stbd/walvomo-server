#ifndef PERSISTENT_USER_STORAGE_HPP_
#define PERSISTENT_USER_STORAGE_HPP_

#include "../Globals.hpp"

namespace PersistentStatisticsDb
{
	void init(const std::string & dbName, const std::string & dbUsername, const std::string & dbPassword);
	void storeUser(const WGlobals::UserInformation & user);
	void addVotelist(const std::string user, const unsigned listId);
	void incrementVotelistViewed(const std::string user, const unsigned listId);
	void shutdown(void);

}
#endif
