#include "CouchSession.hpp"
#include "Logger.hpp"

#include "db/UserInfo.pb.h"
#include "db/RepresentativeInfo.pb.h"
#include "db/VoteInfo.pb.h"
#include "db/PoliticalSeason.pb.h"
#include "db/SiteNews.pb.h"
#include "db/NewsElements.pb.h"

#include <boost/thread.hpp>
#include <stdexcept>
#include <ctime>
#include <cstdio>

using namespace W;

//Static functions declared in this file
static void couchbaseGetCallback(lcb_t instance, const void * cookie, lcb_error_t error, const lcb_get_resp_t * response);
static CouchSession * getRelatedSession(lcb_t instance);
static void couchbaseErrorCallback(lcb_t instance, lcb_error_t error, const char * errinfo);
static void couchbaseStorageCallback(lcb_t instance, const void *, lcb_storage_t , lcb_error_t error, const lcb_store_resp_t *resp);

namespace W
{
	static const char * const COUCHBASE_HOSTNAME = "localhost:8091";
	static const char * const COUCHBASE_USERNAME = 0;
	static const char * const COUCHBASE_PASSWORD = 0;
	static const char * const COUCHBASE_BUCKETNAME = "default";

	static const unsigned NUMBER_OF_COUCHBASE_SESSIONS = 10;

	struct CouchBaseConnections
	{
		std::vector<std::pair<bool, lcb_t> > sessions;
		boost::timed_mutex mutex;

		CouchBaseConnections() {

			for (unsigned i = 0; i < NUMBER_OF_COUCHBASE_SESSIONS; ++i) {
				lcb_t instance = 0;
				lcb_create_st options;
				memset(&options, 0, sizeof(options));
				options.version = 0;
				options.v.v0.user = COUCHBASE_USERNAME;
				options.v.v0.host = COUCHBASE_HOSTNAME;
				options.v.v0.passwd = COUCHBASE_PASSWORD;
				options.v.v0.bucket = COUCHBASE_BUCKETNAME;

				if (lcb_create(&instance, &options) != LCB_SUCCESS) {
					throw std::runtime_error ("Could not create Couchbase instance");
				}

				lcb_set_store_callback(instance, couchbaseStorageCallback);
				lcb_set_error_callback(instance, couchbaseErrorCallback);
				lcb_set_get_callback(instance, couchbaseGetCallback);

				if (lcb_connect(instance) != LCB_SUCCESS) {
					W::Log::error() << "Error connecting to CouchBase with error code: " << lcb_get_last_error(instance);
					throw std::runtime_error ("Could not connect to Couchbase");
				}
				lcb_wait(instance);
				this->sessions.push_back(std::make_pair(false, instance));
				W::Log::debug() << "CouchSession " << i << " created";
			}
		}

		~CouchBaseConnections() {
			for (std::vector<std::pair<bool, lcb_t> >::iterator it = this->sessions.begin(); it != this->sessions.end(); ++it) {
				lcb_destroy(it->second);
				W::Log::debug() << "CouchSession released";
			}
		}


		lcb_t & getSession(CouchSession * couchSession) {//FIXME - ottaa pointterin sessiion
			boost::unique_lock<boost::timed_mutex> lock(this->mutex);
			while (1) {
				for (std::vector<std::pair<bool, lcb_t> >::iterator it = this->sessions.begin(); it != this->sessions.end(); ++it) {
					if (!it->first) {
						it->first = true;
						lcb_set_cookie(it->second, (void *)couchSession);
#ifdef DEBUG
						W::Log::debug() << "Reserving connection to couchdb took: " << std::distance(this->sessions.begin(), it) << " tries";
#endif
						return it->second;
					}
				}
				W::Log::warn() << "Could not get free db session, yielding";
				boost::thread::yield();
			}
			throw std::runtime_error ("getSession failed");
		}

		void release(lcb_t & session) {
			boost::unique_lock<boost::timed_mutex> lock(this->mutex);
			for (std::vector<std::pair<bool, lcb_t> >::iterator it = this->sessions.begin(); it != this->sessions.end(); ++it) {
				if (it->second == session) {
					it->first = false;
					return;
				}
			}
		}
	};
	static CouchBaseConnections couchBaseConnections;

	struct CouchConnection {
		CouchConnection(lcb_t & connection) : connection(connection) {}
		~CouchConnection(void) {couchBaseConnections.release(connection);}
		lcb_t & connection;
	};

	struct SessionInformation
	{
		SessionInformation(void)
			: cas(0), error(LCB_SUCCESS)
		{
			unstructuredData.second.reserve(COUCHSESSION_DATA_BUFFER_SIZE);
		}

		private:
			std::pair<bool, std::string> unstructuredData;
			std::pair<bool, std::string> usersPerCountry;
			std::pair<bool, std::string> userRef;
			std::pair<bool, WDbType::UserInfoContainer> userInfo;
			std::pair<bool, WDbType::ListOfVotesForMonth> votesForMonth;
			std::pair<bool, WDbType::VoteInfo> votesInfo;
			std::pair<bool, WDbType::RepresentativeInfo> representativeInfo;
			std::pair<bool, WDbType::PoliticalSeasonSeating> seasonSeatingInfo;
			std::pair<bool, WDbType::PoliticalSeasonsList> politicalSeasonsListInfo;
			std::pair<bool, WDbType::PoliticalSeason> politicalSeasonInfo;
			std::pair<bool, WDbType::UserVoteList> userVoteList;
			std::pair<bool, WDbType::NewsIndex> newsIndex;
			std::pair<bool, WDbType::NewsItem> newsItem;
			std::pair<bool, WDbType::PoliticalPartyNames> partyNameInfo;
			std::pair<bool, WDbType::VoteRecord> voteRecordInfo;
			std::pair<bool, WDbType::Dictionary> voteStatistics;
			std::pair<bool, WDbType::Collections> collections;
			std::pair<bool, WDbType::CollectionOfUpdateSources> collectionOfUpdateSources;
			std::pair<bool, WDbType::UpdateSource> updateSource;
			std::pair<bool, WDbType::UpdateItem> updateItem;

			lcb_cas_t cas;
			lcb_error_t error;

		friend void ::couchbaseGetCallback(lcb_t instance, const void * cookie, lcb_error_t error, const lcb_get_resp_t * response);
		friend void ::couchbaseStorageCallback(lcb_t instance, const void *, lcb_storage_t , lcb_error_t error, const lcb_store_resp_t *resp);
		friend class CouchSession;
	};
}

template<class T>
bool parseObjectFromStream(T & t, const char * bytes, const unsigned size)
{
	t.second.Clear();
	if (size != 0) {
		if (!t.second.ParseFromArray((char *)bytes, size)) {
			t.second.Clear();
			return false;
		}
	} else {
		t.first = true;
		return true;
	}
	t.first = true;
	return true;
}

static CouchSession * getRelatedSession(lcb_t instance)
{
	const void * cookie = 0;
	if ((cookie = lcb_get_cookie(instance)) == 0) {
		W::Log::error() << "Could not find cookie for instance in getRelatedSession";
		throw std::invalid_argument ("Could not find cookie for instance");
	}
	return static_cast<CouchSession *>((void *)cookie);
}

static void couchbaseErrorCallback(lcb_t , lcb_error_t error, const char * errinfo)
{
	W::Log::error() << "couchbaseErrorCallback called with error: " << errinfo << " (" << error << ")";
}

static void couchbaseStorageCallback(lcb_t instance, const void *, lcb_storage_t ,
		lcb_error_t error, const lcb_store_resp_t *resp)
{
	CouchSession * session = getRelatedSession(instance);
	SessionInformation * sessionInfo = session->getSessionInformation();
	sessionInfo->cas = resp->v.v0.cas;
	sessionInfo->error = error;
}

static void couchbaseGetCallback(lcb_t instance, const void * cookie, lcb_error_t error, const lcb_get_resp_t * response)
{
	if ((error != LCB_SUCCESS) && (error != LCB_KEY_ENOENT)) {
		W::Log::error() << "Error at couchbaseGetCallback: " << error;
		return;
	}

	CouchSession * session = getRelatedSession(instance);
	SessionInformation * sessionInfo = session->getSessionInformation();
	DbRequestType::DbRequestType * type = (DbRequestType::DbRequestType *)cookie;


#ifdef DEBUG
	W::Log::debug() << "couchbaseGetCallback for get type: " << *type << " size of value: " << response->v.v0.nbytes << " flags: " << response->v.v0.flags << " cas: " << response->v.v0.cas;
#endif

	sessionInfo->cas = response->v.v0.cas;
	switch (*type) {
		case DbRequestType::GET_USER_INFO:
			if (!parseObjectFromStream(sessionInfo->userInfo, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse userInfo, size: " << response->v.v0.nbytes;
			}
			break;

		case DbRequestType::GET_VOTES_LIST:
			if (!parseObjectFromStream(sessionInfo->votesForMonth, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse votesForMonth, size: " << response->v.v0.nbytes;
			}
			break;

		case DbRequestType::GET_VOTE:
			if (!parseObjectFromStream(sessionInfo->votesInfo, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse votesInfo, size: " << response->v.v0.nbytes;
			}
			break;

		case DbRequestType::GET_REPRESENTATIVE:
			if (!parseObjectFromStream(sessionInfo->representativeInfo, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse representativeInfo, size: " << response->v.v0.nbytes;
			}
			break;

		case DbRequestType::GET_SEASON_SEATING:
			if (!parseObjectFromStream(sessionInfo->seasonSeatingInfo, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse seasonSeatingInfo, size: " << response->v.v0.nbytes;
			}
			break;
		case DbRequestType::GET_POLITICAL_SEASONS_LIST:
			if (!parseObjectFromStream(sessionInfo->politicalSeasonsListInfo, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse politicalSeasonsListInfo, size: " << response->v.v0.nbytes;
			}
			break;
		case DbRequestType::GET_POLITICAL_SEASON:
			if (!parseObjectFromStream(sessionInfo->politicalSeasonInfo, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse politicalSeasonInfo, size: " << response->v.v0.nbytes;
			}
			break;
		case DbRequestType::GET_USER_VOTE_LIST:
			if (!parseObjectFromStream(sessionInfo->userVoteList, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse userVoteList, size: " << response->v.v0.nbytes;
			}
			break;
		case DbRequestType::GET_UNSTRUCTURED_DATA:
			sessionInfo->unstructuredData.second.assign((char *)response->v.v0.bytes, response->v.v0.nbytes);
			sessionInfo->unstructuredData.first = true;
			break;
		case DbRequestType::GET_USER_REF:
			sessionInfo->userRef.second.assign((char *)response->v.v0.bytes, response->v.v0.nbytes);
			sessionInfo->userRef.first = true;
			break;
		case DbRequestType::GET_NEWS_INDEX:
			if (!parseObjectFromStream(sessionInfo->newsIndex, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse newsIndex, size: " << response->v.v0.nbytes;
			}
			break;
		case DbRequestType::GET_NEWS_ITEM:
			if (!parseObjectFromStream(sessionInfo->newsItem, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse newsItem, size: " << response->v.v0.nbytes;
			}
			break;
		case DbRequestType::GET_PARTY_NAME_INFO:
			if (!parseObjectFromStream(sessionInfo->partyNameInfo, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse partyNameInfo, size: " << response->v.v0.nbytes;
			}
			break;
		case DbRequestType::GET_VOTE_RECORD:
			if (!parseObjectFromStream(sessionInfo->voteRecordInfo, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse voteRecordInfo, size: " << response->v.v0.nbytes;
			}
			break;
		case DbRequestType::GET_VOTE_STATISTICS:
			if (!parseObjectFromStream(sessionInfo->voteStatistics, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse voteStatistics, size: " << response->v.v0.nbytes;
			}
			break;
		case DbRequestType::GET_COLLECTIONS:
			if (!parseObjectFromStream(sessionInfo->collections, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse voteRecordInfo, size: " << response->v.v0.nbytes;
			}
			break;
		case DbRequestType::GET_COLLECTION:
			if (!parseObjectFromStream(sessionInfo->collectionOfUpdateSources, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse voteRecordInfo, size: " << response->v.v0.nbytes;
			}
			break;
		case DbRequestType::GET_UPDATE_SOURCE:
			if (!parseObjectFromStream(sessionInfo->updateSource, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse voteRecordInfo, size: " << response->v.v0.nbytes;
			}
			break;
		case DbRequestType::GET_UPDATE_ITEM:
			if (!parseObjectFromStream(sessionInfo->updateItem, (char *)response->v.v0.bytes, response->v.v0.nbytes)) {
				W::Log::error() << "Failed to parse voteRecordInfo, size: " << response->v.v0.nbytes;
			}
			break;
		default:
			W::Log::error() << "Unhandled type in couchbaseGetCallback, type: " << *type;
			break;
	}
}

CouchSession::CouchSession(void)
	: DatabaseSession(), sessionInformation(0)
{
	this->sessionInformation = new SessionInformation;
	W::Log::debug() << "CouchSession created";
}

CouchSession::~CouchSession(void)
{
	if (this->sessionInformation) {delete this->sessionInformation; this->sessionInformation = 0;}
	W::Log::debug() << "CouchSession released";
}

void CouchSession::connectToDatabase()
{
}

bool CouchSession::createNewUser(const char * username, const size_t usernameLength)
{
	static const unsigned tagLength = strlen(DB_USERNAME_TAG);
	const unsigned keyLength = tagLength + usernameLength;
	const unsigned valueLength = strlen(DEFAULT_FIELD_VALUE);

	WDbType::UserInfoContainer container;
	container.set_version(DB_USERINFO_VERSION);
	container.mutable_userinfo()->set_username(username, usernameLength);
	container.mutable_userinfo()->set_password(DEFAULT_FIELD_VALUE, valueLength);
	container.mutable_userinfo()->set_passwordsalt(DEFAULT_FIELD_VALUE, valueLength);
	container.mutable_userinfo()->set_passwordhashalgorithm(DEFAULT_FIELD_VALUE, valueLength);
	container.mutable_userinfo()->set_latestloginattempt(std::time(NULL));
	container.mutable_userinfo()->set_numberoffailedloginattempsaftersuccessfullogin(0);
	container.mutable_userinfo()->set_userstatus(WDbType::UserInfo_UserStatus_NORMAL);
	container.mutable_userinfo()->set_email(DEFAULT_FIELD_VALUE, valueLength);
	container.mutable_userinfo()->mutable_emailtoken()->set_tokenhash(DEFAULT_FIELD_VALUE, valueLength);
	container.mutable_userinfo()->mutable_emailtoken()->set_emailtokenrole(WDbType::UserInfo_EmailTokenRole_NOT_USED);
	container.mutable_userinfo()->mutable_emailtoken()->set_tokenexpirationtime(0);
	container.mutable_userinfo()->set_userinfoversion(0);
	container.mutable_userinfo()->set_gender(WDbType::UserInfo::GENDER_NOT_SET);
	container.mutable_userinfo()->set_yearofbirth(0);

	if (!container.IsInitialized()) {
		W::Log::error() << "createNewUser: Initializing container failed";
		return false;
	}

	memcpy(this->keyBuffer, DB_USERNAME_TAG, tagLength);
	memcpy(this->keyBuffer + tagLength, username, usernameLength);
	this->keyBuffer[keyLength] = '\0';

	return this->writeObjectToDb(this->keyBuffer, keyLength, container, 0);
}

bool CouchSession::setUserData(WGlobals::UserInformation & user)
{
	if (!user.IsInitialized()) {
		W::Log::error() << "createNewUser: Not all fields were set: " << user.has_username() << user.has_password() << user.has_passwordsalt() << user.has_passwordhashalgorithm();
		return false;
	}

	static const unsigned tagLength = strlen(DB_USERNAME_TAG);
	const unsigned keyLength = tagLength + user.username().length();
	WDbType::UserInfoContainer container;

	container.set_version(DB_USERINFO_VERSION);
	*container.mutable_userinfo() = user;

	memcpy(this->keyBuffer, DB_USERNAME_TAG, tagLength);
	memcpy(this->keyBuffer + tagLength, user.username().c_str(), user.username().length());
	this->keyBuffer[keyLength] = '\0';

	if (!container.IsInitialized() || !container.mutable_userinfo()->IsInitialized()) {
		W::Log::error() << "setUserData: Not all fields were set in container";
		return false;
	}

	if (this->writeObjectToDb(this->keyBuffer, keyLength, container, user.userinfoversion())) {
		user.set_userinfoversion(this->sessionInformation->cas);
		return true;
	} else {
		return false;
	}
}

bool CouchSession::validateUser(const WGlobals::UserInformation & )
{
	return true;
}

bool CouchSession::userExists(const char * username, const size_t & usernameLength)
{
	const WGlobals::UserInformation & userInfo = this->findUser(username, usernameLength);
	if (userInfo.has_username()) {
		return true;
	} else {
		return false;
	}
}

const WGlobals::UserInformation & CouchSession::findUser(const char * username, const size_t & usernameLength)
{
	if (usernameLength > WGlobals::USERNAME_MAX_LENGTH) {
		std::string temp(username, usernameLength);
		W::Log::warn() << "CouchSession::findUser: username too long: " << temp << " of size " << usernameLength;
		throw std::invalid_argument ("findUser: username too long");
	}

	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_USER_INFO;
	static const unsigned tagLength = strlen(DB_USERNAME_TAG);
	const unsigned keyLength = tagLength + usernameLength;

	memcpy(this->keyBuffer, DB_USERNAME_TAG, tagLength);
	memcpy(this->keyBuffer + tagLength, username, usernameLength);
	this->keyBuffer[keyLength] = '\0';

	readObjectFromDb(this->keyBuffer, keyLength, requestType, this->sessionInformation->userInfo);
	this->sessionInformation->userInfo.second.mutable_userinfo()->set_userinfoversion(this->sessionInformation->cas);
	return *this->sessionInformation->userInfo.second.mutable_userinfo();
}

bool CouchSession::deleteUser(const WGlobals::UserInformation & user)
{

	CouchConnection dbSession(couchBaseConnections.getSession(this));
    lcb_remove_cmd_t cmd1;
    lcb_remove_cmd_t cmd2;
    const lcb_remove_cmd_t * const commands[] = {&cmd1, &cmd2};
    memset(&cmd1, 0, sizeof(cmd1));
    memset(&cmd2, 0, sizeof(cmd2));

	//User
	static const unsigned userTagLength = strlen(DB_USERNAME_TAG);
	const unsigned userKeyLength = userTagLength + user.username().length();
	memcpy(this->keyBuffer, DB_USERNAME_TAG, userTagLength);
	memcpy(this->keyBuffer + userTagLength, user.username().c_str(), user.username().length());
	this->keyBuffer[userKeyLength] = '\n';
    cmd1.v.v0.key = this->keyBuffer;
    cmd1.v.v0.nkey = userKeyLength;

	//Email ref
	static const unsigned emailTagLength = strlen(DB_USER_REF);
	const unsigned emailKeyLength = emailTagLength + user.email().length();
	memcpy(this->keyBuffer, DB_USER_REF, emailTagLength);
	memcpy(this->keyBuffer + emailTagLength, user.email().c_str(), user.email().length());
	this->keyBuffer[emailKeyLength] = '\n';
    cmd2.v.v0.key = this->keyBuffer;
    cmd2.v.v0.nkey = emailKeyLength;

	if (lcb_remove(dbSession.connection, 0, 2, commands) != LCB_SUCCESS) {
		W::Log::warn() << "Could not delete email in deleteUser, key: " << this->keyBuffer;
	}

	//Identity ref
	static const unsigned identityTagLength = strlen(DB_USER_REF);
	for (::google::protobuf::RepeatedPtrField<WDbType::UserInfo_Identity>::const_iterator it = user.identities().begin();
			it != user.identities().end(); ++it) {
		std::string identityKey = it->provider() + ":" + it->id();
		const unsigned identityKeyLength = identityTagLength + identityKey.length();
		memcpy(this->keyBuffer, DB_USER_REF, identityTagLength);
		memcpy(this->keyBuffer + identityTagLength, identityKey.c_str(), identityKey.length());
		this->keyBuffer[identityKeyLength] = '\n';

		lcb_remove_cmd_t cmd;
	    const lcb_remove_cmd_t * const commands[] = {&cmd};
	    memset(&cmd, 0, sizeof(cmd));

		if (lcb_remove(dbSession.connection, 0, 1, commands) != LCB_SUCCESS) {
			W::Log::warn() << "Could not delete identity in deleteUser, key: " << this->keyBuffer;
		}
	}
	return true;
}

const WGlobals::ListOfVotesForMonth & CouchSession::findVotesForMonth(const unsigned year, const unsigned month)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_VOTES_LIST;
	static const unsigned tagLength = strlen(DB_VOTES_LIST_TAG);

	memcpy(this->keyBuffer, DB_VOTES_LIST_TAG, tagLength);
	int bytes = sprintf(this->keyBuffer + tagLength, "%d.%d", year, month); //Adds last null to array
	if (bytes == 0) {
		W::Log::error() << "findVotesForMonth: Error writing with sprintf, return: " << bytes;
		throw std::runtime_error ("findVotesForMonth: sprintf");
	}

	const unsigned keyLength = tagLength + bytes;
	W::Log::info() << "Searching for votes for month with key: " << this->keyBuffer << " (" << keyLength << ")";

	readObjectFromDb(this->keyBuffer, keyLength, requestType, this->sessionInformation->votesForMonth);
	return this->sessionInformation->votesForMonth.second;
}

const WGlobals::VoteInformation & CouchSession::findVote(const char * voteId, const size_t idLength)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_VOTE;
	static const unsigned tagLength = strlen(DB_VOTE_TAG);

	memcpy(this->keyBuffer, DB_VOTE_TAG, tagLength);
	memcpy(this->keyBuffer + tagLength, voteId, idLength);
	const unsigned keyLength = tagLength + idLength;
	*(this->keyBuffer + keyLength) = '\0';

	readObjectFromDb(this->keyBuffer, keyLength, requestType, this->sessionInformation->votesInfo);
	return this->sessionInformation->votesInfo.second;

}

const WGlobals::PoliticalSeasonSeating & CouchSession::findSeatingForSeason(const unsigned seasonStartingYear)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_SEASON_SEATING;
	static const unsigned tagLength = strlen(DB_SEASON_SEATING_TAG);

	memcpy(this->keyBuffer, DB_SEASON_SEATING_TAG, tagLength);
	int bytes = sprintf(this->keyBuffer + tagLength, "%d", seasonStartingYear); //Adds last null to array
	if (bytes == 0) {
		W::Log::error() << "findSeatingForSeason: Error writing with sprintf, return: " << bytes;
		throw std::runtime_error ("findSeatingForSeason: sprintf");
	}

	const unsigned keyLength = tagLength + bytes;
	readObjectFromDb(this->keyBuffer, keyLength, requestType, this->sessionInformation->seasonSeatingInfo);
	return this->sessionInformation->seasonSeatingInfo.second;
}

const WGlobals::RepresentativeInfo & CouchSession::findRepresentativeInfo(const unsigned representativeId)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_REPRESENTATIVE;
	static const unsigned tagLength = strlen(DB_REPRESENTATIVE_TAG);

	memcpy(this->keyBuffer, DB_REPRESENTATIVE_TAG, tagLength);
	int bytes = sprintf(this->keyBuffer + tagLength, "%d", representativeId); //Adds last null to array
	if (bytes == 0) {
		W::Log::error() << "findRepresentativeInfo: Error writing with sprintf, return: " << bytes;
		throw std::runtime_error ("findRepresentativeInfo: sprintf");
	}

	const unsigned keyLength = tagLength + bytes;

	readObjectFromDb(this->keyBuffer, keyLength, requestType, this->sessionInformation->representativeInfo);
	return this->sessionInformation->representativeInfo.second;
}

const WGlobals::PoliticalSeasonsList & CouchSession::findPoliticalSeasonsList(void)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_POLITICAL_SEASONS_LIST;
	static const unsigned tagLength = strlen(DB_POLITICAL_SEASON_LIST_TAG);

	memcpy(this->keyBuffer, DB_POLITICAL_SEASON_LIST_TAG, tagLength);
	const unsigned keyLength = tagLength;
	*(this->keyBuffer + keyLength) = '\0';

	readObjectFromDb(this->keyBuffer, keyLength, requestType, this->sessionInformation->politicalSeasonsListInfo);
	return this->sessionInformation->politicalSeasonsListInfo.second;
}

const WGlobals::PoliticalSeason & CouchSession::findPoliticalSeason(const unsigned seasonStartingYear)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_POLITICAL_SEASON;
	static const unsigned tagLength = strlen(DB_POLITICAL_SEASON_INFO_TAG);

	memcpy(this->keyBuffer, DB_POLITICAL_SEASON_INFO_TAG, tagLength);
	int bytes = sprintf(this->keyBuffer + tagLength, "%d", seasonStartingYear); //Adds last null to array
	if (bytes == 0) {
		W::Log::error() << "findRepresentativeInfo: Error writing with sprintf, return: " << bytes;
		throw std::runtime_error ("findRepresentativeInfo: sprintf");
	}

	const unsigned keyLength = tagLength + bytes;

	readObjectFromDb(this->keyBuffer, keyLength, requestType, this->sessionInformation->politicalSeasonInfo);
	return this->sessionInformation->politicalSeasonInfo.second;
}

const WGlobals::UserVoteList & CouchSession::findUserVoteList(const char * username, const unsigned usernameLength, const unsigned listId)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_USER_VOTE_LIST;
	static const unsigned tagLength = strlen(DB_USER_VOTE_LIST_TAG);

	memcpy(this->keyBuffer, DB_USER_VOTE_LIST_TAG, tagLength);
	memcpy(this->keyBuffer + tagLength, username, usernameLength);
	int bytes = sprintf(this->keyBuffer + tagLength + usernameLength, ".%d", listId); //Adds last null to array
	if (bytes == 0) {
		W::Log::error() << "findUserVoteList: Error writing with sprintf, return: " << bytes;
		throw std::runtime_error ("findUserVoteList: sprintf");
	}
	const unsigned keyLength = tagLength + usernameLength + bytes;
	readObjectFromDb(this->keyBuffer, keyLength, requestType, this->sessionInformation->userVoteList);
	return this->sessionInformation->userVoteList.second;
}

void CouchSession::setUserVoteList(const char * username, const unsigned usernameLength, const unsigned listId, const WGlobals::UserVoteList & userVoteList)
{
	static const unsigned tagLength = strlen(DB_USER_VOTE_LIST_TAG);
	memcpy(this->keyBuffer, DB_USER_VOTE_LIST_TAG, tagLength);
	memcpy(this->keyBuffer + tagLength, username, usernameLength);
	int bytes = sprintf(this->keyBuffer + tagLength + usernameLength, ".%d", listId); //Adds last null to array
	if (bytes == 0) {
		W::Log::error() << "findUserVoteList: Error writing with sprintf, return: " << bytes;
		throw std::runtime_error ("findUserVoteList: sprintf");
	}

	const unsigned keyLength = tagLength + usernameLength + bytes;
	this->writeObjectToDb(this->keyBuffer, keyLength, userVoteList, 0);
}

void CouchSession::setUserReferece(const char * username, const unsigned usernameLength, const char * key, const unsigned keyLength)
{
	static const unsigned tagLength = strlen(DB_USER_REF);
	memcpy(this->keyBuffer, DB_USER_REF, tagLength);
	memcpy(this->keyBuffer + tagLength, key, keyLength);
	const unsigned refKeyLength = tagLength + keyLength;
	this->keyBuffer[refKeyLength] = '\0';
	memcpy(this->dataBuffer, username, usernameLength);

	W::Log::debug() << "Inserting object with key: " << this->keyBuffer << " (" << refKeyLength << ") length of data: " << usernameLength;

	CouchConnection dbSession(couchBaseConnections.getSession(this));
    lcb_store_cmd_t cmd;
    const lcb_store_cmd_t * const commands[] = { &cmd };

    memset(&cmd, 0, sizeof(cmd));
    cmd.v.v0.key = this->keyBuffer;
    cmd.v.v0.nkey = refKeyLength;
    cmd.v.v0.bytes = this->dataBuffer;
    cmd.v.v0.nbytes = usernameLength;
    cmd.v.v0.operation = LCB_SET;

	lcb_error_t errorCode = lcb_store(dbSession.connection, 0, 1, commands);
	if (errorCode != LCB_SUCCESS) {
		W::Log::error() << "writeObjectToDb libcouchbase_store failed with error code " << errorCode;
	} else {
		lcb_wait(dbSession.connection);//TODO: make sure this actually blocks until request has been completed fully
	}
}

const WGlobals::UserInformation & CouchSession::findUserByReferece(const char * key, const unsigned keyLength)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_USER_REF;
	static const unsigned tagLength = strlen(DB_USER_REF);
	memcpy(this->keyBuffer, DB_USER_REF, tagLength);
	memcpy(this->keyBuffer + tagLength, key, keyLength);
	const unsigned refKeyLength = tagLength + keyLength;
	this->keyBuffer[refKeyLength] = '\0';

	this->sessionInformation->userInfo.second.Clear();
	this->sessionInformation->userRef.first = false;
	readObjectFromDb(this->keyBuffer, refKeyLength, requestType, this->sessionInformation->userRef);
	if (this->sessionInformation->userRef.first && !this->sessionInformation->userRef.second.empty()) {
		return this->findUser(this->sessionInformation->userRef.second.c_str(), this->sessionInformation->userRef.second.length());
	} else {
		return *this->sessionInformation->userInfo.second.mutable_userinfo();
	}
}

const WGlobals::NewsIndex & CouchSession::findNewsIndex(void)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_NEWS_INDEX;
	static const unsigned tagLength = strlen(DB_NEWS_INDEX_TAG);
	memcpy(this->keyBuffer, DB_NEWS_INDEX_TAG, tagLength);
	this->keyBuffer[tagLength] = '\0';
	readObjectFromDb(this->keyBuffer, tagLength, requestType, this->sessionInformation->newsIndex);
	return this->sessionInformation->newsIndex.second;
}

const WGlobals::NewsItem & CouchSession::findNewsItem(const char * key, const unsigned keyLength)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_NEWS_ITEM;
	static const unsigned tagLength = strlen(DB_NEWS_ITEM_TAG);
	memcpy(this->keyBuffer, DB_NEWS_ITEM_TAG, tagLength);
	memcpy(this->keyBuffer + tagLength, key, keyLength);
	const unsigned refKeyLength = tagLength + keyLength;
	this->keyBuffer[refKeyLength] = '\0';

	readObjectFromDb(this->keyBuffer, refKeyLength, requestType, this->sessionInformation->newsItem);
	return this->sessionInformation->newsItem.second;
}

const WGlobals::PoliticalPartyNames & CouchSession::findPoliticalPartyNameInfo(void)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_PARTY_NAME_INFO;
	static const unsigned tagLength = strlen(DB_PARTY_NAME_INFO_TAG);
	memcpy(this->keyBuffer, DB_PARTY_NAME_INFO_TAG, tagLength);
	this->keyBuffer[tagLength] = '\0';

	readObjectFromDb(this->keyBuffer, tagLength, requestType, this->sessionInformation->partyNameInfo);
	return this->sessionInformation->partyNameInfo.second;
}

const WGlobals::VoteRecord & CouchSession::findVoteRecord(const char * date, const unsigned dateLength)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_VOTE_RECORD;
	static const unsigned tagLength = strlen(DB_VOTE_RECORD_TAG);
	memcpy(this->keyBuffer, DB_VOTE_RECORD_TAG, tagLength);
	memcpy(this->keyBuffer + tagLength, date, dateLength);
	const unsigned refKeyLength = tagLength + dateLength;
	this->keyBuffer[refKeyLength] = '\0';

	this->sessionInformation->voteRecordInfo.second.Clear();
	readObjectFromDb(this->keyBuffer, refKeyLength, requestType, this->sessionInformation->voteRecordInfo);
	return this->sessionInformation->voteRecordInfo.second;
}

const WGlobals::VoteStatistics & CouchSession::findVoteStatistics(const char * voteId, const unsigned idLength)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_VOTE_STATISTICS;
	static const unsigned tagLength = strlen(DB_VOTE_STATISTICS_TAG);
	memcpy(this->keyBuffer, DB_VOTE_STATISTICS_TAG, tagLength);
	memcpy(this->keyBuffer + tagLength, voteId, idLength);
	const unsigned refKeyLength = tagLength + idLength;
	this->keyBuffer[refKeyLength] = '\0';

	this->sessionInformation->voteStatistics.second.Clear();
	readObjectFromDb(this->keyBuffer, refKeyLength, requestType, this->sessionInformation->voteStatistics);
	return this->sessionInformation->voteStatistics.second;
}

const WGlobals::Collections & CouchSession::findCollections(void)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_COLLECTIONS;
	static const unsigned tagLength = strlen(DB_COLLECTIONS_TAG);
	memcpy(this->keyBuffer, DB_COLLECTIONS_TAG, tagLength);
	this->keyBuffer[tagLength] = '\0';

	this->sessionInformation->collections.second.Clear();
	readObjectFromDb(this->keyBuffer, tagLength, requestType, this->sessionInformation->collections);
	return this->sessionInformation->collections.second;
}

const WGlobals::CollectionOfUpdateSources & CouchSession::findCollectionOfUpdateSources(const char * name, const unsigned nameLength)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_COLLECTION;
	static const unsigned tagLength = strlen(DB_COLLECTION_TAG);
	memcpy(this->keyBuffer, DB_COLLECTION_TAG, tagLength);
	memcpy(this->keyBuffer + tagLength, name, nameLength);
	const unsigned refKeyLength = tagLength + nameLength;
	this->keyBuffer[refKeyLength] = '\0';

	this->sessionInformation->collectionOfUpdateSources.second.Clear();
	readObjectFromDb(this->keyBuffer, refKeyLength, requestType, this->sessionInformation->collectionOfUpdateSources);
	return this->sessionInformation->collectionOfUpdateSources.second;
}

const WGlobals::UpdateSource & CouchSession::findUpdateSource(const char * name, const unsigned nameLength)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_UPDATE_SOURCE;
	static const unsigned tagLength = strlen(DB_UPDATE_SOURCE_TAG);
	memcpy(this->keyBuffer, DB_UPDATE_SOURCE_TAG, tagLength);
	memcpy(this->keyBuffer + tagLength, name, nameLength);
	const unsigned refKeyLength = tagLength + nameLength;
	this->keyBuffer[refKeyLength] = '\0';

	this->sessionInformation->updateSource.second.Clear();
	readObjectFromDb(this->keyBuffer, refKeyLength, requestType, this->sessionInformation->updateSource);
	return this->sessionInformation->updateSource.second;
}

const WGlobals::UpdateItem & CouchSession::findUpdateItem(const char * baseName, const unsigned baseNameLength, const unsigned newsNumber)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_UPDATE_ITEM;
	static const unsigned tagLength = strlen(DB_UPDATE_ITEM_TAG);
	memcpy(this->keyBuffer, DB_UPDATE_ITEM_TAG, tagLength);
	memcpy(this->keyBuffer + tagLength, baseName, baseNameLength);
	unsigned refKeyLength = tagLength + baseNameLength;

	int bytes = sprintf(this->keyBuffer + refKeyLength, ":%d", newsNumber); //Adds last null to array
	if (bytes == 0) {
		W::Log::error() << "findUpdateItem: Error writing with sprintf, return: " << bytes;
		throw std::runtime_error ("findUpdateItem: sprintf");
	}
	refKeyLength += bytes;
	this->keyBuffer[refKeyLength] = '\0';

	this->sessionInformation->updateItem.second.Clear();
	readObjectFromDb(this->keyBuffer, refKeyLength, requestType, this->sessionInformation->updateItem);
	return this->sessionInformation->updateItem.second;
}

bool CouchSession::setUnstructuredData(const char * key, const unsigned keyLength, const char * data, const unsigned dataLength, unsigned expirationLength)
{
	bool returnValue = true;
	CouchConnection dbSession(couchBaseConnections.getSession(this));
	memcpy(this->keyBuffer, key, keyLength);
	this->keyBuffer[keyLength] = '\0';

    lcb_store_cmd_t cmd;
    const lcb_store_cmd_t * const commands[] = { &cmd };

    memset(&cmd, 0, sizeof(cmd));
    cmd.v.v0.key = this->keyBuffer;
    cmd.v.v0.nkey = keyLength;
    cmd.v.v0.bytes = data;
    cmd.v.v0.nbytes = dataLength;
    cmd.v.v0.operation = LCB_ADD;

	lcb_error_t errorCode = lcb_store(dbSession.connection, 0, 1, commands);
	if (errorCode != LCB_SUCCESS) {
		W::Log::error() << "writeObjectToDb libcouchbase_store failed with error code " << errorCode;
		returnValue = false;
	} else {
		lcb_wait(dbSession.connection);//TODO: make sure this actually blocks until request has been completed fully
		returnValue = true;
	}
	return returnValue;
}

bool CouchSession::findUnstructuredData(const char * key, const unsigned keyLength, std::string & data)
{
	static const enum DbRequestType::DbRequestType requestType = DbRequestType::GET_UNSTRUCTURED_DATA;
	memcpy(this->keyBuffer, key, keyLength);
	this->keyBuffer[keyLength] = '\0';

	readObjectFromDb(this->keyBuffer, keyLength, requestType, this->sessionInformation->unstructuredData);
	if (!this->sessionInformation->unstructuredData.first) {
		return false;
	} else {
		data.assign(this->sessionInformation->unstructuredData.second);
		return true;
	}
}

bool CouchSession::clearUnstructuredData(const char * key, const unsigned keyLength)
{
	CouchConnection dbSession(couchBaseConnections.getSession(this));
	lcb_remove_cmd_t cmd;
	const lcb_remove_cmd_t * const commands[] = {&cmd};
	memset(&cmd, 0, sizeof(cmd));
	cmd.v.v0.key = key;
	cmd.v.v0.nkey = keyLength;

	lcb_error_t errorCode = lcb_remove(dbSession.connection, 0, 1, commands);
	if (errorCode != LCB_SUCCESS) {
		W::Log::error() << "clearUnstructuredData failed with reason: " << errorCode;
		return false;
	}
	return true;
}

template<class T>
bool CouchSession::writeObjectToDb(const char * key, const unsigned keyLength, T & t, const uint64_t dataVersion)
{
	bool returnValue = true;
	const size_t serializedSize = t.ByteSize();

	if (serializedSize > COUCHSESSION_DATA_BUFFER_SIZE) {
		W::Log::error() << "writeObjectToDb: Unable to fit object to data buffer, size: " << serializedSize;
		return false;
	}

	if (!t.SerializeToArray(this->dataBuffer, serializedSize)) {
		W::Log::error() << "writeDb: Error serializing object";
		return false;
	}

	W::Log::info() << "Inserting object with key: " << key << " (" << keyLength << " bytes), size of value: " << serializedSize << " cas: " << dataVersion;
	CouchConnection dbSession(couchBaseConnections.getSession(this));
	lcb_error_t errorCode = LCB_SUCCESS;
    lcb_store_cmd_t cmd;
    const lcb_store_cmd_t * const commands[] = { &cmd };

    memset(&cmd, 0, sizeof(cmd));
    cmd.v.v0.key = key;
    cmd.v.v0.nkey = keyLength;
    cmd.v.v0.bytes = this->dataBuffer;
    cmd.v.v0.nbytes = serializedSize;

	if (dataVersion != 0) {
	    cmd.v.v0.operation = LCB_REPLACE;
		errorCode = lcb_store(dbSession.connection, 0, 1, commands);
	} else {
	    cmd.v.v0.operation = LCB_SET;
	    errorCode = lcb_store(dbSession.connection, 0, 1, commands);
	}
	if ((errorCode != LCB_SUCCESS) || (this->sessionInformation->error != LCB_SUCCESS)) {
		W::Log::error() << "writeObjectToDb libcouchbase_store failed with error codes: "
				<< errorCode << " " << this->sessionInformation->error;
		returnValue = false;
	} else {
		lcb_wait(dbSession.connection);//TODO: make sure this actually blocks until request has been completed fully
		returnValue = true;
	}
	return returnValue;
}

template<class T>
bool CouchSession::readObjectFromDb(const char * key, const unsigned keyLength, DbRequestType::DbRequestType requestType, T & t)
{
	bool returnValue = true;
	W::Log::info() << "readDb reading with key " << key << " (" << keyLength << " bytes)";

	CouchConnection dbSession(couchBaseConnections.getSession(this));
	lcb_get_cmd_t cmd;
	const lcb_get_cmd_t *commands[] = {&cmd};
	memset(&cmd, 0, sizeof(cmd));
	cmd.v.v0.key = key;
	cmd.v.v0.nkey = keyLength;

	lcb_error_t errorCode = lcb_get(dbSession.connection, &requestType, 1, commands);
	if (errorCode != LCB_SUCCESS) {
		W::Log::error() << "readObjectToDb mget failed with reason: " << errorCode;
		returnValue = false;
	} else {
		t.first = false;
		lcb_wait(dbSession.connection);//TODO: make sure this actually blocks until request has been completed fully
		if (!t.first) {
			throw std::runtime_error ("readObjectFromDb: flag was not set");
		}
		returnValue = true;
	}
	return returnValue;
}

