#include "Application.hpp"
#include "DatabaseSession.hpp"
#include "CouchSession.hpp"
#include "Logger.hpp"
#include "ContentManager.hpp"
#include "UserSession.hpp"
#include "VotesAnalyserWidget.hpp"
#include "DialogWidgets.hpp"
#include "Globals.hpp"
#include "db/SiteNews.pb.h"
#include "RssAggregatorWidget.hpp"
#include "DataCache.hpp"
#include "db/PersistentStatisticsStorage.hpp"

#include <google/protobuf/stubs/common.h>

#include <stdexcept>
#include <Wt/WText>
#include <Wt/WContainerWidget>
#include <Wt/WStackedWidget>
#include <Wt/WMenu>
#include <Wt/WTemplate>
#include <Wt/WEnvironment>
#include <Wt/Auth/AuthWidget>
#include <Wt/Auth/Login>
#include <Wt/Auth/PasswordVerifier>
#include <Wt/Auth/PasswordStrengthValidator>
#include <Wt/Auth/HashFunction>
#include <Wt/Auth/RegistrationModel>
#include <Wt/WPushButton>
#include <Wt/WRandom>
#include <Wt/WServer>

#include <locale>

using namespace W;

Wt::Auth::AuthService Application::authService;
Wt::Auth::PasswordService Application::passwordService(Application::authService);
Wt::Auth::GoogleService Application::googleOAuth(Application::authService);

static const unsigned MainPageContentId = ContentManager::getInstance()->registerContent("root/static/html/main.html");
static const unsigned FrontPageContentId = ContentManager::getInstance()->registerContent("root/static/html/frontpage.html");

#include <Wt/Auth/RegistrationWidget>
#include <Wt/WLineEdit>
#include <Wt/WSelectionBox>
#include <Wt/WComboBox>
namespace W
{

	class RegistrationModel : public Wt::Auth::RegistrationModel {

		public:
			static const Wt::WFormModel::Field ChooseGender;
			static const Wt::WFormModel::Field YearOfBirth;
			static const Wt::WFormModel::Field Disclaimer;
			static const char * const Locale;
			static const std::string InvalidChars;

			RegistrationModel(const Wt::Auth::AuthService & baseAuth, Wt::Auth::AbstractUserDatabase & users, Wt::Auth::Login & login, Wt::WObject * parent = 0)
			: Wt::Auth::RegistrationModel(baseAuth, users, login, parent)
			{
				this->setEmailPolicy(Wt::Auth::RegistrationModel::EmailMandatory);
				addField(ChooseGender, Wt::WString::tr("Wt.Auth.gender.info"));
				addField(YearOfBirth, Wt::WString::tr("Wt.Auth.year-of-birth.info"));
				addField(Disclaimer, Wt::WString::tr("Wt.Auth.disclaimer.info"));
			}

			bool isVisible(Wt::WFormModel::Field field) const
			{
				if (field == ChooseGender) {
					return true;
				}
				if (field == YearOfBirth) {
					return true;
				}
				if (field == Disclaimer) {
					return true;
				}
				return Wt::Auth::RegistrationModel::isVisible(field);
			}

			void reset(void)
			{
				Wt::Auth::RegistrationModel::reset();
				this->setEmailPolicy(Wt::Auth::RegistrationModel::EmailMandatory);
				//addField(ChooseGender, Wt::WString::tr("Wt.Auth.gender.info"));
			}

			Wt::Auth::User doRegister(void)
			{
				//HaX
				bool ok = true;
				UserSession & userSession = static_cast<UserSession &>(this->users());
				Wt::Auth::User user = Wt::Auth::RegistrationModel::doRegister();

				WDbType::UserInfo_Gender gender = this->parseGenderInfo();
				if (gender != WDbType::UserInfo::GENDER_NOT_SET) {
					userSession.setGender(user, gender);
				} else {
					W::Log::error() << "Could not parse gender info from register req";
					ok = false;
				}
				unsigned age = 0;
				if (this->parseYearOfBirthInfo(age)) {
					userSession.setYearOfBirth(user, age);
				} else {
					W::Log::error() << "Could not parse age info from register req";
					ok = false;
				}

				if (ok) {
					userSession.finalizeRegisteration(user);
					return user;
				} else {
					userSession.deleteUser(user);
					return Wt::Auth::User();
				}
			}

			bool validateString(const std::string & string) {
				static std::locale loc(RegistrationModel::Locale);
				for (std::string::const_iterator it = string.begin(); it != string.end(); ++it) {
					if (std::iscntrl(*it, loc) ||
						std::isspace(*it, loc) ||
						(RegistrationModel::InvalidChars.find(*it) != std::string::npos)) {
						return false;
					}
				}
				return true;
			}

			bool validateField(Field field)
			{
				if (field == Wt::Auth::FormBaseModel::LoginNameField) {
					const std::string username = this->valueText(Wt::Auth::FormBaseModel::LoginNameField).toUTF8();
					if (!this->validateString(username)) {
						setValidation(field, Wt::WValidator::Result(Wt::WValidator::Invalid, Wt::WString::tr("Wt.Auth.string.invalid")));
						return false;
					}
					if (username.length() > WGlobals::USERNAME_MAX_LENGTH) {
						setValidation(field, Wt::WValidator::Result(Wt::WValidator::Invalid, Wt::WString::tr("Wt.Auth.name.too-long")));
						return false;
					}
				} else if (field == Wt::Auth::RegistrationModel::ChoosePasswordField) {
					const std::string password = this->valueText(Wt::Auth::RegistrationModel::ChoosePasswordField).toUTF8();
					if (!this->validateString(password)) {
						setValidation(field, Wt::WValidator::Result(Wt::WValidator::Invalid, Wt::WString::tr("Wt.Auth.string.invalid")));
						return false;
					}
				} else if (field == Wt::Auth::RegistrationModel::RepeatPasswordField) {
					const std::string password = this->valueText(Wt::Auth::RegistrationModel::RepeatPasswordField).toUTF8();
					if (!this->validateString(password)) {
						setValidation(field, Wt::WValidator::Result(Wt::WValidator::Invalid, Wt::WString::tr("Wt.Auth.string.invalid")));
						return false;
					}
				} else if (field == Wt::Auth::RegistrationModel::EmailField) {
					const std::string email = this->valueText(Wt::Auth::RegistrationModel::EmailField).toUTF8();
					if (!this->validateString(email)) {
						setValidation(field, Wt::WValidator::Result(Wt::WValidator::Invalid, Wt::WString::tr("Wt.Auth.string.invalid")));
						return false;
					}
				} else if (field == RegistrationModel::Disclaimer) {
					setValid(field);
					return true;
				} else if (field == RegistrationModel::YearOfBirth) {
					unsigned dummy = 0;//FIXME
					if (this->parseYearOfBirthInfo(dummy)) {
						setValid(field);
						return true;
					} else {
						setValidation(field, Wt::WValidator::Result(Wt::WValidator::Invalid));
						return false;
					}
				} else if (field == RegistrationModel::ChooseGender) {
					if (this->parseGenderInfo() != WDbType::UserInfo::GENDER_NOT_SET) {
						setValid(field);
						return true;
					} else {
						setValidation(field, Wt::WValidator::Result(Wt::WValidator::Invalid));
						return false;
					}
				}
				return Wt::Auth::RegistrationModel::validateField(field);
			}

			WDbType::UserInfo_Gender parseGenderInfo(void) {
				static const Wt::WString stringMale = Wt::WString::tr("Wt.Auth.gender.male");
				static const Wt::WString stringFemale = Wt::WString::tr("Wt.Auth.gender.female");
				if (stringMale == this->valueText(ChooseGender)) {
					return WDbType::UserInfo::MALE;
				} else if (stringFemale == this->valueText(ChooseGender)) {
					return WDbType::UserInfo::FEMALE;
				} else {
					return WDbType::UserInfo::GENDER_NOT_SET;
				}
			}

			bool parseYearOfBirthInfo(unsigned & age) {
				std::string strAge = this->valueText(YearOfBirth).toUTF8();
				if (!this->validateString(strAge)) {
					return false;
				}
				if (strAge.length() != 4) {
					return false;
				}
				if (WGlobals::convertStrToUInt(strAge, age)) {
					if ((age > 1900) && (age < 2012)) {
						return true;
					}
				}
				return false;
			}
	};
	const Wt::WFormModel::Field RegistrationModel::ChooseGender = "choose-gender";
	const Wt::WFormModel::Field RegistrationModel::YearOfBirth = "year-of-birth";
	const Wt::WFormModel::Field RegistrationModel::Disclaimer = "disclaimer";
	const char * const RegistrationModel::Locale = "en_US.UTF-8";
	const std::string RegistrationModel::InvalidChars(";*");

	class RegistrationView : public Wt::Auth::RegistrationWidget {
		public:
			RegistrationView(Wt::Auth::AuthWidget * authWidget)
			: Wt::Auth::RegistrationWidget(authWidget)
			{
				setTemplateText(Wt::WString::tr("Wt.Auth.template.registration"));
			}

			Wt::WFormWidget * createFormWidget(Wt::WFormModel::Field field)
			{
				if (field == RegistrationModel::ChooseGender) {
					Wt::WComboBox * genderCombo = new Wt::WComboBox();
					genderCombo->addItem(Wt::WString::tr("Wt.Auth.gender.male"));
					genderCombo->addItem(Wt::WString::tr("Wt.Auth.gender.female"));
					genderCombo->setStyleClass("register-gender");
					return genderCombo;
				} else if (field == RegistrationModel::YearOfBirth) {
					return new Wt::WLineEdit();
				} else if (field == RegistrationModel::Disclaimer) {
					Wt::WPushButton * button = new Wt::WPushButton(Wt::WString::tr("Wt.Auth.disclaimer.button"));
					button->setStyleClass("register-disclaimer-button");
					button->clicked().connect(this, &RegistrationView::showTermsOfUsage);
					return button;
				} else {
					return Wt::Auth::RegistrationWidget::createFormWidget(field);
				}
			}

			void showTermsOfUsage(void) {
				W::showTermsOfUsage();
			}
	};

	class WAuthWidget : public Wt::Auth::AuthWidget
	{
		public:
			WAuthWidget(const Wt::Auth::AuthService & baseAuth, Wt::Auth::AbstractUserDatabase & users, Wt::Auth::Login & login, Wt::WContainerWidget  * parent = 0)
				: Wt::Auth::AuthWidget(baseAuth, users, login, parent)
			{}
			virtual ~WAuthWidget(void) {}

		protected:
			Wt::Auth::RegistrationModel * createRegistrationModel()
			{
				Wt::Auth::RegistrationModel *result = new RegistrationModel(*model()->baseAuth(),	model()->users(), login(), this);

				if (model()->passwordAuth())
					result->addPasswordAuth(model()->passwordAuth());

				result->addOAuth(model()->oAuth());
				return result;
			}

			Wt::WWidget * createRegistrationView(const Wt::Auth::Identity & id)
			{
				RegistrationView * w = new RegistrationView(this);
				Wt::Auth::RegistrationModel * model = createRegistrationModel();

				if (id.isValid())
					model->registerIdentified(id);

				w->setModel(model);
				return w;
			}
	};

	class NewsItemWidget : public Wt::WContainerWidget
	{
		public:
		NewsItemWidget(Wt::WContainerWidget * parent = 0)
				: Wt::WContainerWidget(parent)
			{
				this->setStyleClass("fp-news");
				this->title = new Wt::WText(this);
				this->title->setStyleClass("fp-news-item-title");
				new Wt::WBreak(this);
				this->published = new Wt::WText(this);
				this->published->setStyleClass("fp-news-item-published");
				new Wt::WBreak(this);
				this->content = new Wt::WText(this);
				this->content->setStyleClass("fp-news-item-content");
			}

			void setShownNewsData(const WGlobals::NewsItem & newsItem)
			{
				this->title->setText(Wt::WString::fromUTF8(newsItem.title()));
				this->published->setText(Wt::WString::fromUTF8(newsItem.published()));
				this->content->setText(Wt::WString::fromUTF8(newsItem.content()));
			}

		protected:

		private:
			Wt::WText * title;
			Wt::WText * content;
			Wt::WText * published;
	};

}


Application::Application(const Wt::WEnvironment & env, std::string pathToLocaleFile)
	: Wt::WApplication(env)
{
	if (!pathToLocaleFile.empty()) {
		this->messageResourceBundle().use(pathToLocaleFile, true);
	}
	if (!this->prefix()) {
		throw std::runtime_error ("Failed to init Application");
	}
	this->updatePersistentLoginStatus(env);
	W::Log::info() << "Application started using locale: " << this->locale();
}

Application::Application(const Wt::WEnvironment & env)
	: Wt::WApplication(env)
{
	if (!this->prefix()) {
		throw std::runtime_error ("Failed to init Application");
	}
	this->updatePersistentLoginStatus(env);
	W::Log::info() << "Application started using locale: " << this->locale();
}

Application::~Application(void)
{
	this->postfix();
}

bool Application::prefix(void)
{
	//Wt::WMenu * menu = 0;
	Wt::WTemplate * pageTemplate = 0;
	Wt::WStackedWidget * widgetStack = 0;
	WAuthWidget * authWidget = 0;
	this->buffer.reserve(50);
	Wt::WText * disclamierText = new Wt::WText(Wt::WString::tr("main.disclaimer"));

	this->contentManager = ContentManager::getInstance();
	if ((pageTemplate = new Wt::WTemplate(this->root())) == 0) {
		goto memoryAllocationFailed;
	}
	if ((widgetStack = new Wt::WStackedWidget()) == 0) {
		goto memoryAllocationFailed;
	}
	if ((menu = new Wt::WMenu(widgetStack, Wt::Horizontal)) == 0) {
		goto memoryAllocationFailed;
	}
	if ((this->dbSession = new CouchSession) == 0) {
		goto memoryAllocationFailed;
	}
	if ((this->login = new Wt::Auth::Login) == 0) {
		goto memoryAllocationFailed;
	}
	if ((this->user = new UserSession(this->dbSession)) == 0) {
		goto memoryAllocationFailed;
	}
	if ((this->dataContainer = new DataCache(*this->dbSession)) == 0) {
		goto memoryAllocationFailed;
	}
	if ((this->votesAnalyser = new VotesAnalyser(*this->dataContainer, *this->user, *this->contentManager)) == 0) {
		goto memoryAllocationFailed;
	}
	if ((authWidget = new WAuthWidget(Application::authService, *this->user, *this->login)) == 0) {
		goto memoryAllocationFailed;
	}
	if ((this->newsTable = new Wt::WTable()) == 0) {
		goto memoryAllocationFailed;
	}
	if ((this->rssAggregator = new W::RssAggregatorWidget(*this->dataContainer, *this->contentManager)) == 0) {
		goto memoryAllocationFailed;
	}
	this->login->changed().connect(this, &Application::handleAuthStateChanged);

	this->messageResourceBundle().use("locale/lang");
	this->loadJavascriptLibraries();
	this->useStyleSheet("static/css/main.css");
	this->dbSession->connectToDatabase();//FIXME: if fails, should we have backup page to show?
	this->configureAuthentication(authWidget);

	pageTemplate->setTemplateText(this->contentManager->getContent(MainPageContentId));
	pageTemplate->bindWidget("menu", menu);
	pageTemplate->bindWidget("content", widgetStack);
	pageTemplate->bindWidget("login", authWidget);
	pageTemplate->bindString("contact-info", Wt::WString::tr("main.contact"));
	pageTemplate->bindString("short-legal-info", Wt::WString::tr("main.short-legal-info"));
	pageTemplate->bindWidget("disclaimer-button", disclamierText);
	pageTemplate->bindString("main-share", Wt::WString::tr("main.share"));

	disclamierText->clicked().connect(this, &Application::handleDisclamierClicked);

	this->internalPathChanged().connect(this, &Application::handleInternalPathChanged);
	menu->setInternalPathEnabled("/");
	this->messageResourceBundle().resolveKey("page.frontpage.link", this->buffer);
	menu->addItem(Wt::WString::tr("page.frontpage"), this->createFrontPage())->setPathComponent(this->buffer);
	this->messageResourceBundle().resolveKey("page.analyse.link", this->buffer);
	menu->addItem(Wt::WString::tr("page.analyse"), this->votesAnalyser)->setPathComponent(this->buffer);
	this->messageResourceBundle().resolveKey("page.rssAggregator.link", this->buffer);
	menu->addItem(Wt::WString::tr("page.rssAggregator"), this->rssAggregator)->setPathComponent(this->buffer);

	this->setTitle(Wt::WString::tr("title"));

	this->handleInternalPathChanged(this->internalPath());
	this->doJavaScript(Wt::WString("renderAddthisMainShareButton();").toUTF8());

	return true;

	memoryAllocationFailed:
	W::Log::error() << "Allocating memory in Application::prefix failed";
	if (menu) delete menu;
	if (widgetStack) delete widgetStack;
	if (pageTemplate) delete pageTemplate;
	this->postfix();
	return false;
}

void Application::handleAuthStateChanged()
{
	W::Log::info() << "Auth state changed, isLoggedIn: " << this->login->loggedIn();
	if (!this->login->loggedIn()) {
		this->user->setLoggedInState(false);
		this->user->clear();
		this->clearPersistentLoginStatus();
	} else {
		this->user->setLoggedInState(true);
		if (!this->user->hasValidatedEmail()) {
			W::Log::debug() << "showing showVerifyEmailNotificationDialog";
			showVerifyEmailNotificationDialog();
		}
		this->setPersistentLoginStatusLoggedIn();
	}
	this->votesAnalyser->handleAuthStateChanged();
}

void Application::handleInternalPathChanged(std::string)
{
	W::Log::info() << "Internal path changed to: " << this->internalPath()
			<< " with " << this->environment().getParameterMap().size() << " parameters";

	this->messageResourceBundle().resolveKey("page.analyse.link", this->buffer);
	if (this->internalPathNextPart("/").compare(this->buffer) == 0) {

		bool activated = false;
		if (this->environment().getParameterMap().size() == 2) {
			const std::string * user = this->environment().getParameter("user");
			const std::string * listId = this->environment().getParameter("list");
			unsigned listIdU;
			if (!this->votesAnalyser->isActivated() && (user != 0) && (listId != 0) && WGlobals::convertStrToUInt(*listId, listIdU)) {
				W::Log::info() << "Moving to " << *user << " list " << listIdU;
				this->menu->select(VOTES_WIDGET);
				this->votesAnalyser->setActivated(*user, listIdU);
				activated = true;
			}
		} else if (this->environment().getParameterMap().size() == 1) {
			const std::string * voteId = this->environment().getParameter("voteid");
			if (!this->votesAnalyser->isActivated() && voteId != 0) {
				W::Log::info() << "Moving to vote " << voteId;
				this->votesAnalyser->setActivated(*voteId);
				this->menu->select(VOTES_WIDGET);
				activated = true;
			}
		}
		if (!activated) {
			this->votesAnalyser->setActivated();
		}
	}
	this->messageResourceBundle().resolveKey("page.rssAggregator.link", this->buffer);
	if (this->internalPathNextPart("/").compare(this->buffer) == 0) {
		this->rssAggregator->setActivated();
	}

#ifndef DEBUG
	this->doJavaScript("piwikTrackUser();");
#endif
}

void Application::configure(const Wt::WServer & server)
{
	std::string dbName;
	std::string dbUsername;
	std::string dbPassword;

	Wt::Auth::PasswordVerifier * verifier = 0;
	Wt::Auth::PasswordStrengthValidator * validator = 0;
	if ((verifier = new Wt::Auth::PasswordVerifier()) == 0) {
		goto memoryAllocationFailed;
	}
	if ((validator = new Wt::Auth::PasswordStrengthValidator()) == 0) {
		goto memoryAllocationFailed;
	}
	verifier->addHashFunction(new Wt::Auth::BCryptHashFunction(7));
	Application::authService.setEmailVerificationEnabled(true);
	Application::authService.setAuthTokensEnabled(false);

	Application::passwordService.setVerifier(verifier);
	Application::passwordService.setStrengthValidator(validator);
	Application::passwordService.setAttemptThrottlingEnabled(true);

	server.readConfigurationProperty("persistentdb-dbname", dbName);
	server.readConfigurationProperty("persistentdb-username", dbUsername);
	server.readConfigurationProperty("persistentdb-password", dbPassword);

	if (dbName.empty() || dbUsername.empty() || dbPassword.empty()) {
		throw std::invalid_argument ("Persistant db information not found from configuration");
	}
	PersistentStatisticsDb::init(dbName, dbUsername, dbPassword);
	return ;

    memoryAllocationFailed:
    if (verifier) delete verifier;
    if (validator) delete validator;
    throw std::runtime_error ("Out of memory in Application::configure");
}

void Application::shutdown(void)
{
	google::protobuf::ShutdownProtobufLibrary();
	PersistentStatisticsDb::shutdown();
}

Wt::WTemplate * Application::createFrontPage(void)
{
	Wt::WTemplate * page = new Wt::WTemplate(this->contentManager->getContent(FrontPageContentId));
	page->bindWidget("welcome", new Wt::WText(Wt::WString::tr("fp.welcome")));
	page->bindWidget("newsSectionTitle", new Wt::WText(Wt::WString::tr("fp.news")));
	page->bindWidget("news", this->newsTable);
	page->bindWidget("description", new Wt::WText(Wt::WString::tr("fp.whatisthis")));

	for (unsigned i = 0; i < NUMBER_OF_NEWS_SHOWN_IN_FRONTPAGE; ++i) {
		this->newsTable->elementAt(i, 0)->addWidget(new NewsItemWidget());
		((NewsItemWidget *)this->newsTable->elementAt(i, 0)->widget(0))->hide();
	}
	fillNewsTableWithDataStartingFromIndex();
	return page;
}

void Application::fillNewsTableWithDataStartingFromIndex(int index)
{
	const WGlobals::NewsIndex & newsIndex = this->dbSession->findNewsIndex();
	if (index < newsIndex.newskeys_size()) {
		for (::google::protobuf::RepeatedPtrField<std::string>::const_iterator it = newsIndex.newskeys().begin() + index;
				it != newsIndex.newskeys().end(); ++it) {
			const WGlobals::NewsItem & newsItem = this->dbSession->findNewsItem(it->c_str(), it->length());
			unsigned i = std::distance(newsIndex.newskeys().begin(), it);
			if (i >= NUMBER_OF_NEWS_SHOWN_IN_FRONTPAGE) {
				W::Log::warn() << "Trying to access news table with index " << i;
				break;
			}
			((NewsItemWidget *)this->newsTable->elementAt(i, 0)->widget(0))->setShownNewsData(newsItem);
			((NewsItemWidget *)this->newsTable->elementAt(i, 0)->widget(0))->show();
		}
	} else {
		W::Log::warn() << "fillNewsTableWithDataStartingFromIndex index greater than newsKeys size";
	}
}

void Application::configureAuthentication(Wt::Auth::AuthWidget * authWidget)
{
    authWidget->model()->addPasswordAuth(&Application::passwordService);
    authWidget->model()->addOAuth(&Application::googleOAuth);
    authWidget->setRegistrationEnabled(true);
    authWidget->processEnvironment();
}

void Application::loadJavascriptLibraries(void)
{
	//Wt will complain if file was not found
	Wt::WApplication::instance()->require("static/js/VotesAnalyserLib.js");
	Wt::WApplication::instance()->require("static/js/Util.js");
	Wt::WApplication::instance()->require("static/js/raphael-min.js");
	Wt::WApplication::instance()->require("static/js/g.raphael-min.js");
	Wt::WApplication::instance()->require("static/js/g.bar-min.js");
	Wt::WApplication::instance()->require("static/js/piwik.js");
	Wt::WApplication::instance()->require("//s7.addthis.com/js/300/addthis_widget.js#pubid=xa-50ec4242608cfde7");
}

void Application::postfix(void)
{
	if (this->user) {delete this->user; this->user = 0;}
	if (this->votesAnalyser) {delete this->votesAnalyser; this->votesAnalyser = 0;}
	if (this->login) {delete this->login; this->login = 0;}
	if (this->dataContainer) {delete this->dataContainer; this->dataContainer = 0;}

	//Shared by other classes, so the last one to get deleted
	if (this->dbSession) {delete this->dbSession; this->dbSession = 0;}
}

void Application::updatePersistentLoginStatus(const Wt::WEnvironment & env)
{
	std::string hash;
	if (this->findPersistentAuthCookie(env, hash)) {
		std::string username;
		if (this->dbSession->findUnstructuredData(hash.c_str(), hash.length(), username)) {
			if (username.length() != 0) {
				W::Log::info() << "Found cookie for user " << username;
				Wt::Auth::User user = this->user->findWithId(username);
				if (user.isValid()) {
					W::Log::info() << "Logging user " << username << " with persistent auth";
					this->login->login(user);
				}
			}
		}
	}
}

void Application::clearPersistentLoginStatus(void)
{
	this->removeCookie(WGlobals::COOKIE_SERVICE_NAME);
}

void Application::setPersistentLoginStatusLoggedIn(void)
{
	if (!this->user->isLoggedIn()) {
		W::Log::error() << "setPersistentLoginStatusLoggedIn: user not valid or not logged in";
		return ;
	}

	std::string hash;
	if (this->findPersistentAuthCookie(this->environment(), hash)) {
		std::string username;
		if (!this->dbSession->findUnstructuredData(hash.c_str(), hash.length(), username)) {
			W::Log::error() << "setPersistentLoginStatusLoggedIn: error reading hash from db";
			return ;
		}
		if (username.length() != 0) {
			W::Log::debug() << "Valid hash already exist for user " << username << ". Not updating it";
			return ;
		}
	}
	const WGlobals::UserInformation & user = this->user->getUserInfo();
	if (user.IsInitialized()) {
		std::string hash = Wt::Auth::SHA1HashFunction().compute(Wt::WRandom::generateId(128), Wt::WRandom::generateId(128));
		this->dbSession->setUnstructuredData(hash.c_str(), hash.length(), user.username().c_str(), user.username().length(), WGlobals::COOKIE_TIME_TO_LIVE_SECONDS);
		W::Log::debug() << "Setting hash-username pair with: " << user.username() << " " << hash;
		this->setCookie(WGlobals::COOKIE_SERVICE_NAME, hash, WGlobals::COOKIE_TIME_TO_LIVE_SECONDS);
		return ;
	}
}

bool Application::findPersistentAuthCookie(const Wt::WEnvironment & env, std::string & value)
{
	bool found = false;
	for (std::map<std::string, std::string>::const_iterator it = env.cookies().begin(); it != env.cookies().end(); ++it) {
		if (it->first == WGlobals::COOKIE_SERVICE_NAME && it->second.length()) {
			value.assign(it->second.begin(), it->second.end());
			found = true;
			break;
		}
	}
	return found;
}

void Application::handleDisclamierClicked()
{
	W::showTermsOfUsage();
}
