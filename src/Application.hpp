#ifndef OC_APPLICATION_HPP_
#define OC_APPLICATION_HPP_

#include <Wt/WApplication>
#include <Wt/Auth/AuthService>
#include <Wt/Auth/PasswordService>
#include <Wt/Auth/GoogleService>
#include <Wt/WTable>

#include <string>

namespace Wt
{
	class WStackedWidget;
	class WEnvironment;
	class WText;
	class WMenu;

	namespace Auth
	{
		class AuthWidget;
		class Login;
	}
}

namespace W
{
	class ContentManager;
	class DatabaseSession;
	class UserSession;
	class VotesAnalyser;
	class WAuthWidget;
	class NewsItemWidget;
	class RssAggregatorWidget;
	class DataCache;
}

namespace W {

	const unsigned NUMBER_OF_NEWS_SHOWN_IN_FRONTPAGE = 5;

	enum W_SUBPAGES {
		FRONTPAGE = 0,
		VOTES_WIDGET = 1,
		RSS_WIDGET = 2
	};

	class Application : public Wt::WApplication
	{
		public:
			Application(const Wt::WEnvironment & env);
			Application(const Wt::WEnvironment & env, std::string locale);
			~Application(void);
			static void configure(const Wt::WServer & server);
			static void shutdown(void);

		private:
			Wt::Auth::Login * login;
			ContentManager * contentManager;
			DatabaseSession * dbSession;
			UserSession * user;
			VotesAnalyser * votesAnalyser;
			std::string buffer;
			Wt::WTable * newsTable;
			Wt::WMenu * menu;
			RssAggregatorWidget * rssAggregator;
			DataCache * dataContainer;

			static Wt::Auth::AuthService authService;
			static Wt::Auth::PasswordService passwordService;
			static Wt::Auth::GoogleService googleOAuth;

			Wt::WTemplate * createFrontPage(void);
			void fillNewsTableWithDataStartingFromIndex(int index = 0);
			void handleAuthStateChanged();
			void handleInternalPathChanged(std::string path);
			void configureAuthentication(Wt::Auth::AuthWidget * authWidget);
			void loadJavascriptLibraries(void);
			bool prefix(void);
			void postfix(void);
			void updatePersistentLoginStatus(const Wt::WEnvironment & env);
			void clearPersistentLoginStatus(void);
			void setPersistentLoginStatusLoggedIn(void);
			bool findPersistentAuthCookie(const Wt::WEnvironment & env, std::string & value);
			void handleDisclamierClicked(void);
	};
}

#endif
