#ifndef UserSession_HPP_H
#define USERS_HPP_H

#include "Globals.hpp"
#include "db/UserInfo.pb.h"

#include <Wt/Auth/AbstractUserDatabase>
#include <Wt/Auth/PasswordHash>
#include <string>
#include <ctime>

namespace W
{
	class DatabaseSession;
}

namespace W
{
	static const unsigned USERNAME_BUFFER_SIZE = 50;

	class UserSession : public Wt::Auth::AbstractUserDatabase
	{
		public:
			UserSession(DatabaseSession * session);
			virtual ~UserSession(void);
			Wt::Auth::User registerNew(void);
			Wt::Auth::User findWithId(const std::string & id) const;
			Wt::Auth::User findWithIdentity(const std::string & provider, const Wt::WString & identity) const;
			void addIdentity (const Wt::Auth::User & user, const std::string & provider, const Wt::WString & id);
			void removeIdentity(const Wt::Auth::User & user, const std::string & provider);
			Wt::WString identity (const Wt::Auth::User & user, const std::string & provider) const;
			Wt::Auth::PasswordHash password(const Wt::Auth::User & user) const;
			Wt::WDateTime lastLoginAttempt(const Wt::Auth::User & user) const;
			bool setEmail(const Wt::Auth::User & user, const std::string & address);
			void setEmailToken(const Wt::Auth::User & user, const Wt::Auth::Token & token, Wt::Auth::User::EmailTokenRole role);
			void setUnverifiedEmail(const Wt::Auth::User & user, const std::string & address);
			std::string unverifiedEmail(const Wt::Auth::User & user) const;
			Wt::Auth::User::EmailTokenRole emailTokenRole(const Wt::Auth::User & user) const;
			Wt::Auth::User findWithEmail(const std::string & address) const;
			Wt::Auth::User findWithEmailToken(const std::string& hash) const;
			Wt::Auth::Token emailToken(const Wt::Auth::User & user) const;
			bool saveUserVoteList(const WGlobals::UserVoteList & userVoteList, const unsigned listId);
			bool deleteUserVoteList(const unsigned listId);
			bool getCurrentUserVoteList(const unsigned listId, WGlobals::UserVoteList & userVoteList);
			bool getUserVoteList(const std::string & username, const unsigned listId, WGlobals::UserVoteList & userVoteList);
			unsigned getNewUniqueUserListId(void);
			void setLastLoginAttempt(const Wt::Auth::User & user, const Wt::WDateTime & datetime);
			int failedLoginAttempts(const Wt::Auth::User & user) const;
			void setPassword(const Wt::Auth::User & user, const Wt::Auth::PasswordHash & password);
			void setFailedLoginAttempts(const Wt::Auth::User & user, int count);
			void setLoggedInState(bool loggedIn);
			bool setGender(const Wt::Auth::User & user, const WDbType::UserInfo::Gender gender);
			bool setYearOfBirth(const Wt::Auth::User & user, const unsigned yearOfBirth);
			void deleteUser(const Wt::Auth::User & user);
			bool finalizeRegisteration(const Wt::Auth::User & user);
			inline void clear(void) {this->userInfo.Clear(); this->numberOfLoginAttempts = 0; this->loggedIn = false;}
			inline bool isLoggedIn(void) {return this->loggedIn;}
			inline const WGlobals::UserInformation & getUserInfo(void) {return this->userInfo;}
			inline bool hasValidatedEmail(void) {return this->userInfo.userstatus() == WDbType::UserInfo_UserStatus_NORMAL;}

			/*
			 * Supported auth providers
			 * Zero is the default
			 */
			static std::vector<std::string> supportedAuthService;

		private:
			void commitUserToDb(const Wt::WString & userid) const;
			void commitUserToDb(const Wt::Auth::User & user) const;
			void commitUserToDb(void) const;
			void cleanOldHash(WGlobals::UserInformation & userInfo) const;
			void cleanOldHash(const std::string & hash) const;
			bool isSupportedProvider(const std::string & provider) const;
			bool verifyThatUserIsTheSameInCache(const Wt::Auth::User & user) const ;
			bool verifyThatUserIsTheSameInCache(const Wt::WString & userid) const ;
			void refreshUserInfo(void);
			Wt::Auth::User getUserStatus(const WGlobals::UserInformation & userInfo) const;

			bool loggedIn;
			int numberOfLoginAttempts;
			DatabaseSession * session;
			std::time_t lastLoginAttempTime;
			mutable std::string username;
			mutable WGlobals::UserInformation userInfo;
	};
}

#endif
