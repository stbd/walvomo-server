#include "UserSession.hpp"

#include "Logger.hpp"
#include "DatabaseSession.hpp"
#include "db/VoteInfo.pb.h"
#include "db/PersistentStatisticsStorage.hpp"

#include <cstdlib>

#include <boost/thread.hpp>

using namespace W;

std::vector<std::string> UserSession::supportedAuthService;

/*
 * For initializing static UserSession::supportedAuthService;
 */
static struct GlobalInit
{
	GlobalInit() {
		UserSession::supportedAuthService.push_back("loginname");
		UserSession::supportedAuthService.push_back("google");
	}
} forInitializationOfUserSessionSupportedAuthService;

UserSession::UserSession(DatabaseSession * session)
	: loggedIn(false), session(session)
{
	W::Log::debug() << "UserSession created";
	this->userInfo.Clear();
	this->username.reserve(USERNAME_BUFFER_SIZE);
}

UserSession::~UserSession(void)
{
	if (this->isLoggedIn()) {
		this->commitUserToDb();
	}
	W::Log::debug() << "UserSession destroyed";
}

Wt::Auth::User UserSession::registerNew(void)
{
	W::Log::debug() << "registerNew: creating user: " << this->username;
	if (!this->username.empty() && this->session->createNewUser(this->username.c_str(), this->username.length())) {
		this->userInfo = this->session->findUser(this->username.c_str(), this->username.length());
		if (this->userInfo.IsInitialized()) {
			this->userInfo.set_userstatus(WDbType::UserInfo_UserStatus_WAITING_FOR_CONFORMATION);
			W::Log::info() << "registerNew: successfully registered user " << this->username;
			//PersistentStatisticsDb::storeUser();													//FIXME parameters
			this->loggedIn = true;
			return this->getUserStatus(this->userInfo);
		} else {
			W::Log::error() << "registerNew failed for user " << this->username;
			return Wt::Auth::User();
		}
	}
	W::Log::error() << "registerNew failed, user name was not set or createNewUser failed";
	return Wt::Auth::User();
}

Wt::Auth::User UserSession::findWithId(const std::string & id) const
{
	//FIXME: when is this called by wt?
	W::Log::info() << "findWithId: " << id;
	return this->findWithIdentity("loginname", id);
}

Wt::Auth::User UserSession::findWithIdentity(const std::string & provider, const Wt::WString & identity) const
{
	if (!this->isSupportedProvider(provider)) {
		W::Log::error() << "findWithIdentity does not support searching by " << provider;
		throw std::runtime_error ("findWithIdentity provider not supported");
	}

	if (this->verifyThatUserIsTheSameInCache(identity)) {
		W::Log::debug() << "findWithIdentity: user in cache";
		return Wt::Auth::User(this->userInfo.username(), *this);
	}

	if (supportedAuthService[0].compare(provider) == 0) {
		this->userInfo.Clear();
		this->username = identity.toUTF8();
		this->userInfo = this->session->findUser(this->username.c_str(), this->username.length());
		if (this->userInfo.IsInitialized()) {
			W::Log::debug() << "findWithIdentity: identity " << identity << " by provider "<< provider << " was found";
			return this->getUserStatus(this->userInfo);
		}
	} else {
		this->username = provider + ":" + identity.toUTF8();
		W::Log::debug() << "findWithIdentity: with user ref: " << this->username;
		this->userInfo = this->session->findUserByReferece(this->username.c_str(), this->username.length());
		if (this->userInfo.IsInitialized()) {
			return this->getUserStatus(this->userInfo);
		}
	}
	W::Log::debug() << "findWithIdentity: identity " << identity << " by provider "<< provider << " was not found";
	return Wt::Auth::User();
}

void UserSession::addIdentity(const Wt::Auth::User & user, const std::string & provider, const Wt::WString & id)
{
	if (this->isSupportedProvider(provider) && this->verifyThatUserIsTheSameInCache(user)) {
		for (google::protobuf::RepeatedPtrField< ::WDbType::UserInfo_Identity>::const_iterator it = this->userInfo.identities().begin(); it != this->userInfo.identities().end(); ++it) {
			if (it->provider().compare(provider) == 0) {
				W::Log::warn() << "User: " << user.id() << " all ready has identity by provider " << provider << ", stored id: " << it->id() << " new id: " << id;
				return ;
			}
		}

		WDbType::UserInfo_Identity * identity = this->userInfo.add_identities();
		identity->set_id(id.toUTF8());
		identity->set_provider(provider);
		if (supportedAuthService[0].compare(provider) != 0) {
			this->username = provider + ":" + id.toUTF8();
			this->session->setUserReferece(user.id().c_str(), user.id().length(), this->username.c_str(), this->username.length());
			W::Log::info() << "addIdentity: adding reference to user with " << this->username;
		}
		W::Log::info() << "addIdentity: provider: " << provider <<  " user: " << user.id();
		return ;
	}
	W::Log::error() << "addIdentity: provider not supported or username missmath, username: " << user.id() << " id: " << id << " provider: " << provider;
	throw std::runtime_error ("addIdentity error");
}

Wt::WString UserSession::identity(const Wt::Auth::User & user, const std::string & provider) const
{
	W::Log::debug() << "identity: provider: " << provider << " user: " << user.id();
	if (this->isSupportedProvider(provider) && this->verifyThatUserIsTheSameInCache(user)) {
		for (google::protobuf::RepeatedPtrField< ::WDbType::UserInfo_Identity>::const_iterator it = this->userInfo.identities().begin(); it != this->userInfo.identities().end(); ++it) {
			if (it->provider().compare(provider) == 0) {
				return it->id();
			}
		}
	}
	W::Log::info() << "Identity not found";
	return Wt::WString();
}

void UserSession::removeIdentity(const Wt::Auth::User & user, const std::string & provider)
{
	W::Log::info() << "removeIdentity: for user " << user.id() << " with provider " << provider;
	//Fixme implement
	throw std::runtime_error ("Not implemented: removeIdentity");
}

void UserSession::setPassword(const Wt::Auth::User & user, const Wt::Auth::PasswordHash & password)
{
	W::Log::debug() << "setPassword: for user " << user.id();
	if (this->verifyThatUserIsTheSameInCache(user)) {
		this->userInfo.set_password(password.value());
		this->userInfo.set_passwordsalt(password.value());
		this->userInfo.set_passwordhashalgorithm(password.function());
		this->commitUserToDb(user);
	}
}

Wt::Auth::PasswordHash UserSession::password(const Wt::Auth::User & user) const
{
	W::Log::debug() << "password: for user " << user.id();
	if (this->verifyThatUserIsTheSameInCache(user)) {
		return Wt::Auth::PasswordHash(this->userInfo.passwordhashalgorithm(), this->userInfo.passwordsalt(), this->userInfo.password());
	}
	return Wt::Auth::PasswordHash();
}

int UserSession::failedLoginAttempts(const Wt::Auth::User & user) const
{
	W::Log::debug() << "failedLoginAttempts: for user" << user.id();
	if (this->verifyThatUserIsTheSameInCache(user)) {
		return this->userInfo.numberoffailedloginattempsaftersuccessfullogin();
	}
	return 0;
}

void UserSession::setFailedLoginAttempts(const Wt::Auth::User & user, int count)
{
	W::Log::debug() << "setFailedLoginAttempts: for user " << user.id() << ", count: " << count;
	if (this->verifyThatUserIsTheSameInCache(user)) {
		this->userInfo.set_numberoffailedloginattempsaftersuccessfullogin(count);
		this->commitUserToDb();
		return ;
	}
	W::Log::error() << "setFailedLoginAttempts unsuccessful, user was not set";
}

void UserSession::setLoggedInState(bool loggedIn)
{
	this->loggedIn = loggedIn;
	if (!loggedIn) {
		this->commitUserToDb();
	}
}

bool UserSession::setGender(const Wt::Auth::User & user, const WDbType::UserInfo::Gender gender)
{
	W::Log::debug() << "setGender: user: " << user.id() << " gender: " << (int)gender;
	if (this->verifyThatUserIsTheSameInCache(user)) {
		this->userInfo.set_gender(gender);
		return true;
	}
	W::Log::warn() << "setGender: user not set";
	return false;
}

bool UserSession::setYearOfBirth(const Wt::Auth::User & user, const unsigned yearOfBirth)
{
	W::Log::debug() << "setYearOfBirth: " << user.id() << " " << yearOfBirth;
	if (this->verifyThatUserIsTheSameInCache(user)) {
		this->userInfo.set_yearofbirth(yearOfBirth);
		return true;
	}
	W::Log::warn() << "setYearOfBirth: user not set";
	return false;
}

void UserSession::deleteUser(const Wt::Auth::User & user)
{
	W::Log::debug() << "clearUser: user: " << user.id();
	if (this->verifyThatUserIsTheSameInCache(user)) {
		this->session->deleteUser(this->userInfo);
	}
	W::Log::warn() << "clearUser: user not set";
}

bool UserSession::finalizeRegisteration(const Wt::Auth::User & user)
{
	W::Log::debug() << "finalizeRegisteration: user: " << user.id();
	if (this->verifyThatUserIsTheSameInCache(user)) {
		PersistentStatisticsDb::storeUser(this->userInfo);
		return true;
	}
	W::Log::warn() << "finalizeRegisteration: user not set";
	return false;
}

void UserSession::setLastLoginAttempt(const Wt::Auth::User & user, const Wt::WDateTime & datetime)
{
	W::Log::debug() << "setLastLoginAttempt: " << user.id() << " " << datetime.toString();
	if (this->verifyThatUserIsTheSameInCache(user)) {
		std::time_t timestamp = datetime.toTime_t();
		this->userInfo.set_latestloginattempt(timestamp);
		this->commitUserToDb();
		return ;
	}
	W::Log::error() << "setLastLoginAttempt unsuccessful, user was not set";
}

Wt::WDateTime UserSession::lastLoginAttempt(const Wt::Auth::User & user) const
{
	if (this->verifyThatUserIsTheSameInCache(user)) {
		Wt::WDateTime time;
		time.fromTime_t(this->userInfo.latestloginattempt());
		return time;
	}
	return Wt::WDateTime();
}

bool UserSession::setEmail(const Wt::Auth::User & user, const std::string & address)
{
	W::Log::debug() << "setEmail: for user " << user.id() << " address " << address;
	if (this->verifyThatUserIsTheSameInCache(user) && !address.empty()) {
		this->cleanOldHash(this->userInfo);
		this->session->setUserReferece(this->userInfo.username().c_str(), this->userInfo.username().length(), address.c_str(), address.length());
		this->userInfo.set_email(address);
		this->userInfo.mutable_emailtoken()->set_emailtokenrole(WDbType::UserInfo_EmailTokenRole_NOT_USED);
		this->userInfo.set_userstatus(WDbType::UserInfo_UserStatus_NORMAL);
		this->commitUserToDb();
		return true;
	}
	return false;
}

void UserSession::setUnverifiedEmail(const Wt::Auth::User & user, const std::string & address)
{
	W::Log::debug() << "setUnverifiedEmail: for user " << user.id() << " address " << address;
	if (this->verifyThatUserIsTheSameInCache(user) && !address.empty()) {
		this->cleanOldHash(this->userInfo);
		this->userInfo.set_email(address);
		this->userInfo.set_userstatus(WDbType::UserInfo_UserStatus_WAITING_FOR_CONFORMATION);
		this->commitUserToDb();
		this->session->setUserReferece(this->userInfo.username().c_str(), this->userInfo.username().length(), address.c_str(), address.length());
	}
}

void UserSession::setEmailToken(const Wt::Auth::User & user, const Wt::Auth::Token & token, Wt::Auth::User::EmailTokenRole role)
{
	W::Log::debug() << "setEmailToken: for user " << user.id() << " hash.empty(): " << token.empty() << " hash: " << token.hash();
	if (this->verifyThatUserIsTheSameInCache(user) && !token.empty()) {
		this->cleanOldHash(this->userInfo);
		switch (role) {
			case Wt::Auth::User::VerifyEmail:
				this->userInfo.mutable_emailtoken()->set_emailtokenrole(WDbType::UserInfo_EmailTokenRole_VERIFY_EMAIL);
				this->session->setUnstructuredData(token.hash().c_str(), token.hash().length(), this->userInfo.username().c_str(), this->userInfo.username().length());
				this->userInfo.mutable_emailtoken()->set_tokenexpirationtime(token.expirationTime().toTime_t());
				this->userInfo.mutable_emailtoken()->set_tokenhash(token.hash());
				W::Log::debug() << "setEmailToken: verify";
				break;
			case Wt::Auth::User::LostPassword:
				this->userInfo.mutable_emailtoken()->set_emailtokenrole(WDbType::UserInfo_EmailTokenRole_LOST_PASSWORD);
				this->userInfo.mutable_emailtoken()->set_tokenhash(token.hash());
				this->userInfo.mutable_emailtoken()->set_tokenexpirationtime(token.expirationTime().toTime_t());
				this->session->setUnstructuredData(token.hash().c_str(), token.hash().length(), this->userInfo.username().c_str(), this->userInfo.username().length());
				W::Log::debug() << "setEmailToken: Lost";
				break;
		}
		this->commitUserToDb();
	}
}

std::string UserSession::unverifiedEmail(const Wt::Auth::User & user) const
{
	if (this->verifyThatUserIsTheSameInCache(user)) {
		if (this->userInfo.userstatus() == WDbType::UserInfo_UserStatus_WAITING_FOR_CONFORMATION) {
			return this->userInfo.email();
		}
	}
	return std::string();
}

Wt::Auth::Token UserSession::emailToken(const Wt::Auth::User & user) const
{
	W::Log::debug() << "emailToken: user: " << user.id();
	if (this->verifyThatUserIsTheSameInCache(user) && (this->userInfo.emailtoken().emailtokenrole() != WDbType::UserInfo_EmailTokenRole_NOT_USED)) {
		Wt::WDateTime expirationData;
		expirationData.setTime_t(this->userInfo.emailtoken().tokenexpirationtime());
		return Wt::Auth::Token(this->userInfo.emailtoken().tokenhash(), expirationData);
	}
	W::Log::debug() << "emailToken: not set";
	return Wt::Auth::Token();
}

Wt::Auth::User::EmailTokenRole UserSession::emailTokenRole(const Wt::Auth::User & user) const
{
	W::Log::debug() << "emailTokenRole: for user " << user.id();
	if (this->verifyThatUserIsTheSameInCache(user)) {
		if (this->userInfo.userstatus() == WDbType::UserInfo_UserStatus_WAITING_FOR_CONFORMATION) {
			W::Log::debug() << "emailTokenRole: default verity";
			return Wt::Auth::User::VerifyEmail;
		}
		switch (this->userInfo.mutable_emailtoken()->emailtokenrole()) {
			case WDbType::UserInfo_EmailTokenRole_VERIFY_EMAIL:
				W::Log::debug() << "emailTokenRole: verity";
				return Wt::Auth::User::VerifyEmail;
				break;
			case WDbType::UserInfo_EmailTokenRole_LOST_PASSWORD:
				W::Log::debug() << "emailTokenRole: lost";
				return Wt::Auth::User::LostPassword;
				break;
			default:
				W::Log::error() << "emailTokenRole: role not handled";
				return Wt::Auth::User::VerifyEmail;
		}
	}
	return Wt::Auth::User::VerifyEmail;
}

Wt::Auth::User UserSession::findWithEmail(const std::string & address) const
{
	W::Log::debug() << "findWithEmail: email: " << address;
	this->userInfo = this->session->findUserByReferece(address.c_str(), address.length());
	if (this->userInfo.IsInitialized()) {
		return this->getUserStatus(this->userInfo);
	} else {
		return Wt::Auth::User();
	}
}

Wt::Auth::User UserSession::findWithEmailToken(const std::string & hash) const
{
	W::Log::debug() << "findWithEmailToken: hash: " << hash;
	this->session->findUnstructuredData(hash.c_str(), hash.length(), this->username);
	if (!this->username.empty()) {
		this->cleanOldHash(hash);
		return this->findWithId(this->username);
	}
	W::Log::error() << "findWithEmailToken: user was not found";
	return Wt::Auth::User();
}

bool UserSession::saveUserVoteList(const WGlobals::UserVoteList & userVoteList, const unsigned listId)
{
	if (!this->loggedIn) {
		return false;
	}

	this->refreshUserInfo();
	bool exists = false;
	for (google::protobuf::RepeatedField<google::protobuf::uint32>::const_iterator it = this->userInfo.owneduserlists().begin(); it != this->userInfo.owneduserlists().end(); ++it) {
		if (*it == listId) {
			exists = true;
			break;
		}
	}
	if (!exists) {
		this->userInfo.mutable_owneduserlists()->Add(listId);
	}
	this->session->setUserVoteList(this->userInfo.username().c_str(), this->userInfo.username().length(), listId, userVoteList);
	this->commitUserToDb();
	W::Log::debug() << "saveUserVoteList: listId " << listId << " saved";
	PersistentStatisticsDb::addVotelist(this->userInfo.username(), listId);
	return true;
}

bool UserSession::deleteUserVoteList(const unsigned listId)
{
	if (!this->loggedIn) {
		return false;
	}

	this->refreshUserInfo();
	bool exists = false;
	unsigned index = 0;
	for (google::protobuf::RepeatedField<google::protobuf::uint32>::const_iterator it = this->userInfo.owneduserlists().begin(); it != this->userInfo.owneduserlists().end(); ++it) {
		if (*it == listId) {
			exists = true;
			break;
		}
		++index;
	}
	if (exists) {
		if (index != (unsigned)(this->userInfo.owneduserlists_size()-1)) {
			this->userInfo.mutable_owneduserlists()->SwapElements(index, this->userInfo.owneduserlists_size()-1);
			W::Log::debug() << "Moved " << listId << " from index " << index << " to " << this->userInfo.owneduserlists_size()-1;
		}
		this->userInfo.mutable_owneduserlists()->RemoveLast();
		this->commitUserToDb();
		W::Log::info() << "Deleted uservotelist " << listId;
		return true;
	} else {
		W::Log::error() << "Could not find uservotelist in deleteUserVoteList with listId " << listId;
		return false;
	}
}

bool UserSession::getCurrentUserVoteList(const unsigned listId, WGlobals::UserVoteList & userVoteList)
{
	if (this->loggedIn) {
		return this->getUserVoteList(this->userInfo.username(), listId, userVoteList);
	} else {
		return false;
	}
}

bool UserSession::getUserVoteList(const std::string & username, const unsigned listId, WGlobals::UserVoteList & userVoteList)
{
	userVoteList.Clear();
	userVoteList = this->session->findUserVoteList(username.c_str(), username.length(), listId);
	return userVoteList.IsInitialized();
}

unsigned UserSession::getNewUniqueUserListId(void)
{
	unsigned listId = std::rand();
	ListTrySegment:
	for (google::protobuf::RepeatedField<google::protobuf::uint32>::const_iterator it = userInfo.owneduserlists().begin(); it != userInfo.owneduserlists().end(); ++it) {
		if (*it == listId) {
			goto ListTrySegment;
		}
	}
	return listId;
}

bool UserSession::isSupportedProvider(const std::string & provider) const
{
	for (std::vector<std::string>::const_iterator it = this->supportedAuthService.begin(); it != this->supportedAuthService.end(); ++it) {
		if (it->compare(provider) == 0) {
			return true;
		}
	}
	return false;
}

bool UserSession::verifyThatUserIsTheSameInCache(const Wt::Auth::User & user) const
{
	if (this->userInfo.IsInitialized() && user.isValid() && (this->userInfo.username().compare(user.id()) == 0)) {
		return true;
	}
	return false;
}

bool UserSession::verifyThatUserIsTheSameInCache(const Wt::WString & userid) const
{
	if (this->userInfo.IsInitialized() && this->userInfo.username().compare(userid.toUTF8()) == 0) {
		return true;
	}
	return false;
}

void UserSession::refreshUserInfo(void)
{
	if (this->loggedIn) {
		W::Log::debug() << "Refreshing user info";
		this->userInfo = this->session->findUser(this->userInfo.username().c_str(), this->userInfo.username().length());
		if (!this->userInfo.IsInitialized()) {
			W::Log::error() << "Error refreshing user";
		}
	}
}

void UserSession::commitUserToDb(const Wt::Auth::User & user) const
{
	if (this->verifyThatUserIsTheSameInCache(user) && this->userInfo.IsInitialized()) {
		this->commitUserToDb();
	} else {
		W::Log::error() << "commitUserToDb Error: is current user " << this->verifyThatUserIsTheSameInCache(user) << " is userinfo init " << this->userInfo.IsInitialized();
	}
}

void UserSession::commitUserToDb(const Wt::WString & userid) const
{
	if (this->verifyThatUserIsTheSameInCache(userid) && this->userInfo.IsInitialized()) {
		this->commitUserToDb();
	} else {
		W::Log::error() << "commitUserToDb Error: is current user " << this->verifyThatUserIsTheSameInCache(userid) << " is userinfo init " << this->userInfo.IsInitialized();
	}
}

void UserSession::commitUserToDb(void) const
{
	if (this->userInfo.IsInitialized()) {
		for (unsigned i = 0; i < NUMBER_OF_DB_WRITE_RETIRES; ++i) {
			if (this->session->setUserData(this->userInfo)) {
				break;
			} else {
				this->userInfo = this->session->findUser(this->userInfo.username().c_str(), this->userInfo.username().length());
				unsigned sleeptime = rand() % 100;
				W::Log::warn() << "UserSession could not commit to db, try: " << i+1 << " sleeping: " << sleeptime;
				boost::this_thread::sleep(boost::posix_time::milliseconds(sleeptime));
			}
		}
	} else {
		W::Log::error() << "commitUserToDb (void) Error: is userinfo init " << this->userInfo.IsInitialized();
	}
}

Wt::Auth::User UserSession::getUserStatus(const WGlobals::UserInformation & userInfo) const
{
	Wt::Auth::User user(userInfo.username(), *this);
	//user.addIdentity("loginname", this->userInfo.username());
	//user.setPassword(Wt::Auth::PasswordHash(this->userInfo.passwordhashalgorithm(), this->userInfo.passwordsalt(), this->userInfo.password()));
	/*
	switch (this->userInfo.userstatus()) {
		case WDbType::UserInfo_UserStatus_NORMAL:
			user.setEmail(this->userInfo.email());
			break;
		case WDbType::UserInfo_UserStatus_WAITING_FOR_CONFORMATION:
			user.setUnverifiedEmail(this->userInfo.email());
			break;
		default:
			throw std::runtime_error ("userInfo.status() invalid");
	}
	*/
	return user;
}

void UserSession::cleanOldHash(WGlobals::UserInformation & userInfo) const
{
	if (userInfo.emailtoken().emailtokenrole() != WDbType::UserInfo_EmailTokenRole_NOT_USED) {
		W::Log::debug() << "Cleaning hash: " << this->userInfo.emailtoken().tokenhash();
		this->session->clearUnstructuredData(userInfo.emailtoken().tokenhash().c_str(), userInfo.emailtoken().tokenhash().length());
		//userInfo.mutable_emailtoken()->set_emailtokenrole(WDbType::UserInfo_EmailTokenRole_NOT_USED);
	}
}

void UserSession::cleanOldHash(const std::string & hash) const
{
	W::Log::debug() << "Cleaning hash: " << hash;
	this->session->clearUnstructuredData(hash.c_str(), hash.length());
}
