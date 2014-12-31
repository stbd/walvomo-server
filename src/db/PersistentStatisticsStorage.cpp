#include "PersistentStatisticsStorage.hpp"

#include "UserInfo.pb.h"

#include <mysql/mysql.h>

#include <stdexcept>
#include <string>

using namespace PersistentStatisticsDb;

static std::string DB_USER = "ocdb";
static std::string DB_USER_PASSWD = "OcapP";
static std::string DB = "oc";

static char convertGenderToStr(::WDbType::UserInfo_Gender gender)
{
	if (gender == ::WDbType::UserInfo_Gender_FEMALE) {
		return 'F';
	} else if (gender == ::WDbType::UserInfo_Gender_MALE) {
		return 'M';
	} else {
		throw std::invalid_argument("Gender invalid in convertGenderToStr");
	}
}

void PersistentStatisticsDb::init(const std::string & dbName, const std::string & dbUsername, const std::string & dbPassword)
{
	if (mysql_library_init(0, 0, 0)) {
		throw std::runtime_error ("MySQL init failed");
	}
	DB = dbName;
	DB_USER = dbUsername;
	DB_USER_PASSWD = dbPassword;
}

void PersistentStatisticsDb::addVotelist(const std::string user, const unsigned listId)
{
	MYSQL * connection = 0;
	if ((connection = mysql_init(0)) == 0) {
		throw std::runtime_error ("MySQL init in thread failed");//Exception the right thing?
	}

	if (mysql_real_connect(connection, 0, DB_USER.c_str(), DB_USER_PASSWD.c_str(), DB.c_str(), 0, 0, 0) == 0) {
		throw std::runtime_error ("MySQL real_connect in thread failed");//Exception the right thing?
	}

	std::string listIdStr;
	if (!WGlobals::convertUIntToStr(listId, listIdStr)) {
		throw std::runtime_error ("Failed to convert age to str in storeUser");
	}
	std::string query = "insert into votelist (id, user) values (" + listIdStr + ", \"" + user + "\");";

	mysql_query(connection, query.c_str());
	mysql_close(connection);
}

void PersistentStatisticsDb::incrementVotelistViewed(const std::string user, const unsigned listId)
{
	MYSQL * connection = 0;
	if ((connection = mysql_init(0)) == 0) {
		throw std::runtime_error ("MySQL init in thread failed");//Exception the right thing?
	}

	if (mysql_real_connect(connection, 0, DB_USER.c_str(), DB_USER_PASSWD.c_str(), DB.c_str(), 0, 0, 0) == 0) {
		throw std::runtime_error ("MySQL real_connect in thread failed");//Exception the right thing?
	}

	std::string listIdStr;
	if (!WGlobals::convertUIntToStr(listId, listIdStr)) {
		throw std::runtime_error ("Failed to convert age to str in storeUser");
	}
	std::string query = "update votelist SET viewed=viewed+1 WHERE id=" + listIdStr + " and user=\"" + user + "\";";

	mysql_query(connection, query.c_str());
	mysql_close(connection);
}

void PersistentStatisticsDb::storeUser(const WGlobals::UserInformation & user)
{
	MYSQL * connection = 0;
	if ((connection = mysql_init(0)) == 0) {
		throw std::runtime_error ("MySQL init in thread failed");//Exception the right thing?
	}

	if (mysql_real_connect(connection, 0, DB_USER.c_str(), DB_USER_PASSWD.c_str(), DB.c_str(), 0, 0, 0) == 0) {
		throw std::runtime_error ("MySQL real_connect in thread failed");//Exception the right thing?
	}

	std::string yearofbirth;
	if (!WGlobals::convertUIntToStr(user.yearofbirth(), yearofbirth)) {
		throw std::runtime_error ("Failed to convert age to str in storeUser");
	}
	std::string query = "insert into user (username, gender, email, yearofbirth) values (\"" + user.username() +
					"\", \"" + convertGenderToStr(user.gender()) +
					"\", \"" + user.email() +
					"\", " + yearofbirth +
					")";

	mysql_query(connection, query.c_str());
	mysql_close(connection);
}

void PersistentStatisticsDb::shutdown()
{
	mysql_library_end();
}
