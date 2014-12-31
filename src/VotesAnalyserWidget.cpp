#include "VotesAnalyserWidget.hpp"

#include "DataCache.hpp"
#include "Logger.hpp"
#include "ContentManager.hpp"
#include "db/DbUtils.hpp"
#include "db/RepresentativeInfo.pb.h"
#include "db/PoliticalSeason.pb.h"
#include "UserSession.hpp"
#include "DialogWidgets.hpp"
#include "db/PersistentStatisticsStorage.hpp"

#include <Wt/WPainter>
#include <Wt/WTemplate>
#include <Wt/WContainerWidget>
#include <Wt/WPushButton>
#include <Wt/WText>
#include <Wt/WPaintedWidget>
#include <Wt/WTable>
#include <Wt/WScrollArea>
#include <Wt/WImage>
#include <Wt/WFileResource>
#include <Wt/WJavaScript>
#include <Wt/WDialog>
#include <Wt/WLineEdit>
#include <Wt/WComboBox>
#include <Wt/WTextArea>
#include <Wt/WButtonGroup>
#include <Wt/WRadioButton>
#include <Wt/WEnvironment>
#include <Wt/Utils>

#include <stdexcept>
#include <map>
#include <cstdio>

using namespace W;

static const unsigned VotesAnalyserWidgetContentId = ContentManager::getInstance()->registerContent("root/static/html/VotesAnalyserWidget.html");

namespace W
{
	class RepresentativeInfoWidget : public Wt::WContainerWidget
	{
		public:
			RepresentativeInfoWidget(Wt::WContainerWidget * parent = 0)
				: Wt::WContainerWidget(parent)
			{
				this->name = new Wt::WText(this);
				Wt::WImage * closeButton = new Wt::WImage(Wt::WLink("/static/images/icon_remove_medium.png"), this);
				closeButton->setStyleClass("right");
				closeButton->clicked().connect(this, &RepresentativeInfoWidget::hide);
				new Wt::WBreak(this);
				this->party = new Wt::WText(this);
				new Wt::WBreak(this);
				this->region = new Wt::WText(this);
				new Wt::WBreak(this);
				this->link = new Wt::WText(this);
			}

			void setShownRepresentative(const WGlobals::RepresentativeInfo & representativeInfo)
			{
				this->representativeInfo = representativeInfo;
				this->name->setText(Wt::WString::tr("vw.politician.name") + ": " + Wt::WString::fromUTF8(this->representativeInfo.firstname() + " " + this->representativeInfo.lastname()));
				this->party->setText(Wt::WString::tr("vw.politician.party") + ": " + Wt::WString::fromUTF8(WDb::convertPartyEnumToString(representativeInfo.currentpoliticalparty().party())));
				this->region->setText(Wt::WString::tr("vw.politician.region") + ": " + Wt::WString::fromUTF8(representativeInfo.currentpoliticalparty().region()));
				this->link->setText(Wt::WString("<a href=\"{2}\">{1}</a> ").arg(Wt::WString::tr("vw.politician.link")).arg(Wt::Utils::htmlEncode(representativeInfo.infolink())));
			}

		protected:

		private:
			Wt::WText * name;
			Wt::WText * party;
			Wt::WText * region;
			Wt::WText * link;
			WGlobals::RepresentativeInfo representativeInfo;
	};

	class VotesTableElementWidget : public Wt::WContainerWidget
	{
		public:
			VotesTableElementWidget(Wt::WContainerWidget * parent = 0)
				: Wt::WContainerWidget(parent)
			{
				this->header = new Wt::WText(this);
				this->header->setTextFormat(Wt::XHTMLText);
				this->shareVoteButton = new Wt::WPushButton(Wt::WString::tr("vw.share-vote.button"), this);
				this->shareVoteButton->setStyleClass("vaw-vote-share-button");
				this->userChoiseNoButton = new Wt::WPushButton(Wt::WString::tr("vw.userchoise.no"), this);
				this->userChoiseNoButton->setStyleClass("vaw-moveToInterested");
				this->userChoiseYesButton = new Wt::WPushButton(Wt::WString::tr("vw.userchoise.yes"), this);
				this->userChoiseYesButton->setStyleClass("vaw-moveToInterested");
				new Wt::WBreak(this);
				this->voteInfo = new Wt::WText(this);
				this->voteInfo->setTextFormat(Wt::XHTMLText);
				this->voteInfo->setStyleClass("link");

				this->voteInfo->clicked().connect(this, &VotesTableElementWidget::handleClicked);//FIXME test for success
				this->shareVoteButton->clicked().connect(this, &VotesTableElementWidget::handleShareVotePressed);//FIXME test for success
				this->userChoiseNoButton->clicked().connect(this, &VotesTableElementWidget::handleUserChoiseNoButtonClicked);
				this->userChoiseYesButton->clicked().connect(this, &VotesTableElementWidget::handleUserChoiseYesButtonClicked);
			}

			void setVote(const WDbType::VoteInfo & vote, const std::string & voteId, const unsigned relatedSeasonStartingYear)
			{
				this->relatedSeasonStartingYear = relatedSeasonStartingYear;
				this->vote = vote;
				this->header->setText(Wt::WString("{1}:{2} - <a href=\"{3}\">{4}</a>").arg(Wt::WString::fromUTF8(this->vote.dateofvote())).arg((int)this->vote.votenumber()).arg(this->vote.linktorecord()).arg(Wt::WString::tr("vw.vote.link")));

				unsigned choiseNumbers[4];
				for (::google::protobuf::RepeatedPtrField< ::WDbType::Pair >::const_iterator it = vote.votestatistics().pairs().begin();
						it != vote.votestatistics().pairs().end(); ++it) {
					if (WDbType::VoteChoise_Name(WDbType::VoteChoise::YES).compare(it->key()) == 0) {
						WGlobals::convertStrToUInt(it->value(), choiseNumbers[0]);
					} else if (WDbType::VoteChoise_Name(WDbType::VoteChoise::NO).compare(it->key()) == 0) {
						WGlobals::convertStrToUInt(it->value(), choiseNumbers[1]);
					} else if (WDbType::VoteChoise_Name(WDbType::VoteChoise::EMPTY).compare(it->key()) == 0) {
						WGlobals::convertStrToUInt(it->value(), choiseNumbers[2]);
					} else if (WDbType::VoteChoise_Name(WDbType::VoteChoise::AWAY).compare(it->key()) == 0) {
						WGlobals::convertStrToUInt(it->value(), choiseNumbers[3]);
					} else {
						W::Log::warn() << "Value of vote statistics key was not valid, value was " << it->key();
					}
				}
				Wt::WString statLine = Wt::WString("<br /><div class=\"vaw-statline\"> {1}: {2}: {3}, {4}: {5}, {6}: {7}, {8}: {9}, <u>{10}</u></div>").arg(Wt::WString::tr("vw.vote.statistics-line")).
						arg(Wt::WString::tr("vw.vote.yes")).arg(choiseNumbers[0]).
						arg(Wt::WString::tr("vw.vote.no")).arg(choiseNumbers[1]).
						arg(Wt::WString::tr("vw.vote.empty")).arg(choiseNumbers[2]).
						arg(Wt::WString::tr("vw.vote.away")).arg(choiseNumbers[3]).
						arg(choiseNumbers[0] > choiseNumbers[1] ? Wt::WString::tr("vw.vote.approved") : Wt::WString::tr("vw.vote.not-approved"));

				this->voteInfo->setText(Wt::WString::fromUTF8("<div style=\"text-decoration: underline\">" + this->vote.shortdescription()) + "</div>" + statLine + "<br /><div class=\"main-horizontalRule\"></div>");
				this->setStyleClass("vaw-vote-list-element-no-choise");
			}

			void setVote(const WDbType::VoteInfo & vote, const std::string & voteId, const unsigned relatedSeasonStartingYear, bool choise)
			{
				setVote(vote, voteId, relatedSeasonStartingYear);
				if (choise) {
					this->setStyleClass("vaw-vote-list-element-yes");
				} else {
					this->setStyleClass("vaw-vote-list-element-no");
				}
			}

			Wt::Signal<const VotesTableElementWidget *> & clickedEvent() {return this->clickedEventSignal;}
			Wt::Signal<const VotesTableElementWidget *, bool> & moveToInterestingButtonClicked() {return this->moveToInterestingButtonClickedSignal;}

			inline bool isValid(void) {return this->vote.IsInitialized();}
			inline unsigned getRelatedSeasonStartingYear(void) const {return this->relatedSeasonStartingYear;}
			WGlobals::VoteInformation vote;

		private:
			Wt::Signal<const VotesTableElementWidget *> clickedEventSignal;
			Wt::Signal<const VotesTableElementWidget *, bool> moveToInterestingButtonClickedSignal;
			Wt::WText * header;
			Wt::WText * voteInfo;

			Wt::WPushButton * userChoiseYesButton;
			Wt::WPushButton * userChoiseNoButton;

			Wt::WPushButton * shareVoteButton;
			unsigned relatedSeasonStartingYear;

			void handleClicked(void)
			{
				clickedEventSignal.emit(this);
			}

			void handleUserChoiseNoButtonClicked(void)
			{
				moveToInterestingButtonClickedSignal.emit(this, false);
				this->setStyleClass("vaw-vote-list-element-no");
			}

			void handleUserChoiseYesButtonClicked(void)
			{
				moveToInterestingButtonClickedSignal.emit(this, true);
				this->setStyleClass("vaw-vote-list-element-yes");
			}


			void handleShareVotePressed(void)
			{
				Wt::WString link = this->createVoteLinkToBeShared();
				W::Log::debug() << "Share vote button pressed, creating link: " << link;
				W::showShareDialog(link, link, Wt::WString::tr("vw.share-vote"), Wt::WApplication::instance()->locale());
			}

			Wt::WString createVoteLinkToBeShared(void) {
				std::string voteId;
				WDb::convertDateAndVoteNumToVoteKey(this->vote.dateofvote(), this->vote.votenumber(), voteId);
				return Wt::WString("http://{1}/{2}?{3}={4}").arg(
						Wt::WApplication::instance()->environment().hostName()).arg(
								Wt::WApplication::instance()->bookmarkUrl()).arg("voteid").arg(voteId);
			}
	};

	class InterestedVotesListElementWidget : public Wt::WContainerWidget
	{
		public:
			InterestedVotesListElementWidget(Wt::WContainerWidget * parent = 0)
					: Wt::WContainerWidget(parent), set(false), decision(false)
			{
				this->text = new Wt::WText(this);
				this->text->clicked().connect(this, &InterestedVotesListElementWidget::handleVotenameClicked);
				//this->text->setToolTip("asdasdasdasdasdasd"); //FIXME, maybe
				this->buttonYes = new Wt::WPushButton(Wt::WString::tr("vw.userchoise.yes"), this);
				this->buttonYes->clicked().connect(this, &InterestedVotesListElementWidget::handleYesButtonPressed);
				this->buttonYes->setStyleClass("vaw-userChoise-none");
				this->buttonNo = new Wt::WPushButton(Wt::WString::tr("vw.userchoise.no"), this);
				this->buttonNo->clicked().connect(this, &InterestedVotesListElementWidget::handleNoButtonPressed);
				this->buttonNo->setStyleClass("vaw-userChoise-none");
				Wt::WImage * buttonDelete = new Wt::WImage(Wt::WLink("/static/images/icon_remove_medium.png"), this);
				buttonDelete->setStyleClass("right vaw-interestedListElement-remove-icon");
				buttonDelete->clicked().connect(this, &InterestedVotesListElementWidget::handleDeleteClicked);
				this->setStyleClass("vaw-interestedListElement");
			}

			void setVoteId(const std::string & voteId) {
				this->text->setText(voteId);
				this->id = voteId;
			}

			void resetDecision(void) {
				this->set = false;
				this->buttonYes->setStyleClass("vaw-userChoise-none");
				this->buttonNo->setStyleClass("vaw-userChoise-none");
			}

			const std::string & voteId(void) {
				return this->id;
			}

			bool isEqualVote(const std::string & voteId) {
				return this->id.compare(voteId) == 0;
			}

			inline bool isVoteSet(void) {
				return this->set;
			}

			inline bool userChoise(void) {
				if (this->set) {
					return this->decision;
				} else {
					W::Log::warn() << "userChoise asked when choise not set in InterestedVotesListElementWidget";
					return false;
				}
			}

			void setDecision(bool decision) {
				this->set = true;
				if (decision) {
					this->decision = true;
					this->buttonYes->setStyleClass("vaw-userChoise-chosen");
					this->buttonNo->setStyleClass("vaw-userChoise-none");
				} else {
					this->decision = false;
					this->buttonYes->setStyleClass("vaw-userChoise-none");
					this->buttonNo->setStyleClass("vaw-userChoise-chosen");
				}
			}

			void handleYesButtonPressed(void) {
				this->setDecision(true);
				this->userChoiseChangedOnVoteSignal.emit(this->voteId());
			}

			void handleNoButtonPressed(void) {
				this->setDecision(false);
				this->userChoiseChangedOnVoteSignal.emit(this->voteId());
			}

			void handleVotenameClicked(void) {
				this->clickedEventSignal.emit(this->text);
			}

			void handleDeleteClicked(void) {
				this->deletedEventSignal.emit(this->text);
			}

			Wt::Signal<const Wt::WText *> & clickedEvent() {return this->clickedEventSignal;}
			Wt::Signal<const Wt::WText *> & deletedEvent() {return this->deletedEventSignal;}
			Wt::Signal<std::string> & userChoiseChangedOnVote() {return userChoiseChangedOnVoteSignal;}

		private:
			bool set;
			bool decision;
			Wt::WPushButton * buttonYes;
			Wt::WPushButton * buttonNo;
			Wt::WText * text;
			std::string id;
			Wt::Signal<const Wt::WText *> clickedEventSignal;
			Wt::Signal<const Wt::WText *> deletedEventSignal;
			Wt::Signal<std::string> userChoiseChangedOnVoteSignal;
	};

	class ShownVotesStateContainer
	{
		public:
			ShownVotesStateContainer(void) {
			}

			void setVotesShownForMonth(const unsigned year, const unsigned month, const unsigned seasonStartingYear) {
				this->year = year;
				this->month = month;
				this->seasonStartingYear = seasonStartingYear;
				this->state = SHOW_ALL_VOTES_LIST;
			}

			inline unsigned getShownMonth(void) {return this->month;}
			inline unsigned getShownYear(void) {return this->year;}
			inline unsigned getShownSeasonStartingYear(void) {return this->seasonStartingYear;}
			bool shouldMoveButtonBeShown(void) {return (this->state == SHOW_ALL_VOTES_LIST);}
			bool showAllVotes(void) {return (this->state == SHOW_ALL_VOTES_LIST);}

			void setVotesShownCustomList(const std::string & creatorName, const unsigned listId) {
				this->voteListId.owner = creatorName;
				this->voteListId.listId = listId;
				this->state = SHOW_CUSTOM_LIST;
			}
			inline const struct UserVoteListId & getShowCustomListName(void) {return this->voteListId;}

		private:
			enum {
				SHOW_ALL_VOTES_LIST,
				SHOW_CUSTOM_LIST
			} state;
			unsigned seasonStartingYear;
			unsigned year;
			unsigned month;
			struct UserVoteListId voteListId;
	};
}

VotesAnalyser::VotesAnalyser(DataCache & session, UserSession & userSession, ContentManager & contentManager)
	: activated(false), isVisualizationOperationOngoing(false), userSession(userSession),
	  representativeClikedSignal(this, "representativeCliked"), visualizationCompletedSignal(this, "visualizationCompleted"),
	  dataContainer(session)

{
	Wt::WPushButton * buttonMatch = 0;
	Wt::WPushButton * buttonSave = 0;
	Wt::WPushButton * buttonShare = 0;
	Wt::WPushButton * buttonDelete = 0;
	Wt::WScrollArea * votesListScrollArea = 0;
	Wt::WScrollArea * interestedListScrollArea = 0;
	this->voteDescriptionArea = 0;

	this->ownedVotesIds.reserve(DEFAULT_NUMBER_OF_INTERESTED_VOTES_SHOWN_IN_LIST);
	this->followListsIds.reserve(DEFAULT_NUMBER_OF_INTERESTED_VOTES_SHOWN_IN_LIST);

	//Allocation
	this->buffer.reserve(DEFAULT_BUFFER_SIZE);

	if ((this->shownVotesState = new ShownVotesStateContainer) == 0) {
		goto memmoryAllocationFailed;
	}

	if ((this->voteDescriptionArea = new Wt::WText()) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((this->voteListDescriptionArea = new Wt::WText()) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((this->pageTemplate = new Wt::WTemplate(contentManager.getContent(VotesAnalyserWidgetContentId), this)) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((votesListScrollArea = new Wt::WScrollArea) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((this->ownedVotesLists = new Wt::WComboBox()) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((this->activeMonthList = new Wt::WComboBox()) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((this->followedMonthList = new Wt::WComboBox()) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((interestedListScrollArea = new Wt::WScrollArea) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((this->representativeInfo = new RepresentativeInfoWidget) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((this->votesTable = new Wt::WTable) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((this->interestedVotesTable = new Wt::WTable) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((this->votesRightSideButton = new Wt::WImage(Wt::WLink("/static/images/icon_right_medium.png"))) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((this->votesLeftSideButton = new Wt::WImage(Wt::WLink("/static/images/icon_left_medium.png"))) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((buttonMatch = new Wt::WPushButton(Wt::WString::tr("vw.button.match"))) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((buttonSave = new Wt::WPushButton(Wt::WString::tr("vw.button.save"))) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((buttonShare = new Wt::WPushButton(Wt::WString::tr("vw.button.share"))) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((buttonDelete = new Wt::WPushButton(Wt::WString::tr("vw.button.delete"))) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((this->selectedListDescription = new Wt::WText()) == 0) {
		goto memmoryAllocationFailed;
	}
	if ((this->userVoteList = new WGlobals::UserVoteList) == 0) {
		goto memmoryAllocationFailed;
	}
	this->createVotesListTables();

	this->votesLeftSideButton->setHiddenKeepsGeometry(true);
	this->votesRightSideButton->setHiddenKeepsGeometry(true);
	this->voteDescriptionArea->setHiddenKeepsGeometry(true);

	//Styles
	this->voteListDescriptionArea->setStyleClass("vaw-voteListDescriptionArea");
	this->activeMonthList->setStyleClass("vaw-activeMonthList");
	this->followedMonthList->setStyleClass("vaw-followedMonthList");
	this->votesRightSideButton->setStyleClass("vaw-votesRightSideButton");
	this->selectedListDescription->setStyleClass("vaw-selectedListDescription");
	this->votesLeftSideButton->setStyleClass("vaw-votesLeftSideButton");
	votesListScrollArea->setStyleClass("vaw-votesListScrollArea");
	interestedListScrollArea->setStyleClass("vaw-interestedListScrollArea");
	buttonMatch->setStyleClass("vaw-buttonMatch");
	buttonSave->setStyleClass("vaw-buttonSave");
	buttonShare->setStyleClass("vaw-buttonShare");
	buttonDelete->setStyleClass("vaw-buttonDelete");
	voteDescriptionArea->setStyleClass("vaw-voteDescriptionArea");
	this->ownedVotesLists->setStyleClass("vaw-ownedVotesLists");
	this->representativeInfo->setStyleClass("vaw-representativeInfo");

	//Connect components
	this->representativeClikedSignal.connect(this, &VotesAnalyser::handleRepresentativeCliked);
	this->visualizationCompletedSignal.connect(this, &VotesAnalyser::handleVisualizationDone);
	this->votesRightSideButton->clicked().connect(this, &VotesAnalyser::handleVotesRightSideButtonPressed);
	this->votesLeftSideButton->clicked().connect(this, &VotesAnalyser::handleVotesLeftSideButtonPressed);
	buttonMatch->clicked().connect(this, &VotesAnalyser::handleMatchButtonPressed);
	buttonSave->clicked().connect(this, &VotesAnalyser::handleSaveButtonPressed);
	buttonShare->clicked().connect(this, &VotesAnalyser::handleShareButtonPressed);
	buttonDelete->clicked().connect(this, &VotesAnalyser::handleDeleteButtonPressed);
	this->ownedVotesLists->activated().connect(this, &VotesAnalyser::handleOwnedListsListSelected);
	this->followedMonthList->activated().connect(this, &VotesAnalyser::handleFollowListsListSelected);
	this->activeMonthList->activated().connect(this, &VotesAnalyser::handleActiveMonthsListSelected);

	//Scrollareas
	votesListScrollArea->setScrollBarPolicy(Wt::WScrollArea::ScrollBarAsNeeded);
	votesListScrollArea->setWidget(this->votesTable);
	interestedListScrollArea->setScrollBarPolicy(Wt::WScrollArea::ScrollBarAsNeeded);
	interestedListScrollArea->setWidget(this->interestedVotesTable);

	this->pageTemplate->bindWidget("VoteListDescription", this->voteListDescriptionArea);
	this->pageTemplate->bindWidget("activeMonthList", this->activeMonthList);
	this->pageTemplate->bindWidget("followedMonthList", this->followedMonthList);
	this->pageTemplate->bindWidget("buttonLeft", votesLeftSideButton);
	this->pageTemplate->bindWidget("currentList", this->selectedListDescription);
	this->pageTemplate->bindWidget("buttonRigth", this->votesRightSideButton);
	this->pageTemplate->bindWidget("votes", votesListScrollArea);
	this->pageTemplate->bindWidget("listOfInterestedVotes", this->ownedVotesLists);
	this->pageTemplate->bindWidget("interedtedVotes", interestedListScrollArea);
	this->pageTemplate->bindWidget("matchButton", buttonMatch);
	this->pageTemplate->bindWidget("saveButton", buttonSave);
	this->pageTemplate->bindWidget("shareButton", buttonShare);
	this->pageTemplate->bindWidget("deleteButton", buttonDelete);
	this->pageTemplate->bindWidget("representative", this->representativeInfo);
	this->pageTemplate->bindWidget("description", this->voteDescriptionArea);
	this->pageTemplate->bindString("instructions", Wt::WString::tr("vw.instructions"));

	this->voteListDescriptionArea->hide();
	this->representativeInfo->hide();
	this->voteDescriptionArea->hide();

	return ;

	memmoryAllocationFailed:
	if (votesLeftSideButton) delete votesLeftSideButton;
	if (buttonMatch) delete buttonMatch;
	if (votesListScrollArea) delete votesListScrollArea;
	if (this->userVoteList) {delete this->userVoteList; this->userVoteList = 0;}
	throw std::runtime_error ("Out of memory in VotesAnalyser::VotesAnalyser");
}

VotesAnalyser::~VotesAnalyser(void)
{
	if (this->shownVotesState) {delete this->shownVotesState; this->shownVotesState = 0;}
	if (this->userVoteList) {delete this->userVoteList; this->userVoteList = 0;}
}

void VotesAnalyser::setActivated(void)
{
	if (this->activated) {
		W::Log::debug() << "VotesAnalyser already active in setActivated";
		return;
	}
	W::Log::debug() << "VotesAnalyser set active without parameters";
	if (!this->initialize()) {
		W::showUserErrorDialog(Wt::WString::tr("error.vaw.error.init-failed"));
		return ;
	}
	this->getVotesForMonth(this->shownVotesState->getShownYear(), this->shownVotesState->getShownMonth());
	this->updateWidget();
}

void VotesAnalyser::setActivated(const std::string & username, const unsigned listId)
{
	W::Log::debug() << "VotesAnalyser set active with custom list:" << username << listId;
	if (!this->initialize()) {
		W::showUserErrorDialog(Wt::WString::tr("error.vaw.error.init-failed"));
		return ;
	}
	if (!setUserVoteList(username, listId, *this->userVoteList)) {
		W::showUserErrorDialog(Wt::WString::tr("dialog.error.uservotelist-not-found"));
	} else {
		PersistentStatisticsDb::incrementVotelistViewed(username, listId);
		this->resetcurrentRepresentativeMatchSum();
	}
}

void VotesAnalyser::setActivated(const std::string & voteId)
{
	W::Log::debug() << "VotesAnalyser set active with voteId:" << voteId;
	if (!this->initialize()) {
		W::showUserErrorDialog(Wt::WString::tr("error.vaw.error.init-failed"));
		return ;
	}
	if (!this->showOnlyOneVote(voteId)) {
		W::showUserErrorDialog(Wt::WString::tr("dialog.error.vote-not-found"));
	}
}

void VotesAnalyser::handleAuthStateChanged()
{
	if (!this->activated) {
		W::Log::debug() << "VotesAnalyser::handleAuthStateChanged ignored because widget not init";
		return ;
	}
	this->redrawWidget();
	this->currentUserListInfo.set = false;
	this->currentUserListInfo.listId = 0;
}

void VotesAnalyser::redrawWidget()
{
	this->clearInterestedVotesTable();
	this->setFollowedListsList();
	this->setOwnedListsList();
	this->updateWidget();
}

void VotesAnalyser::handleVoteSelected(const VotesTableElementWidget * voteInfoWidget)
{
	if (this->isVisualizationOperationOngoing) {
		W::Log::debug() << "handleVoteSelected, voteId: " << voteInfoWidget->vote.topic() << "canceled - busy";
		return ;
	}
	W::Log::debug() << "handleVoteSelected, voteId: " << voteInfoWidget->vote.topic()
			<< " date for record: " << voteInfoWidget->vote.dateofvote();


	setSetVoteRecordInformation(voteInfoWidget->vote.dateofvote(), voteInfoWidget->vote.votenumber());//Commented because source sucks
	this->setRepresentativeChoisesToBuffer(this->buffer, voteInfoWidget->vote);
	this->isVisualizationOperationOngoing = true;
	this->doJavaScript("visualizeVoteChoisesWithMap(\"" + this->buffer + "\");");

	this->buffer.clear();
	WDb::convertDateAndVoteNumToVoteKey(voteInfoWidget->vote.dateofvote(), voteInfoWidget->vote.votenumber(), this->buffer);
	const WDbType::Dictionary & stats = this->dataContainer.findVoteStatistics(this->buffer);
	if (stats.IsInitialized()) {
		this->buffer.clear();
		this->setVoteStatisticsToBuffer(stats);
		this->doJavaScript("drawVoteStatistics(" + this->buffer + ");");
	}
}

void VotesAnalyser::handleMoveToInterestingButtonPresed(const VotesTableElementWidget * voteInfoWidget, bool userChoise)
{
	if (this->currentUserListInfo.set) {
		if (voteInfoWidget->getRelatedSeasonStartingYear() != this->currentUserListInfo.relatedSeasonStartingYear) {
			W::showUserErrorDialog(Wt::WString::tr("error.vaw.votes.season.diff"));
			W::Log::debug() << "Season not equal in handleMoveToInterestingButtonPresed";
			return ;
		}
	} else {
		this->currentUserListInfo.setRelatedSeasonStartingYear(voteInfoWidget->getRelatedSeasonStartingYear());
	}

	bool voteDoesNotExistInListAlready = true;
	this->buffer.clear();
	WDb::convertDateAndVoteNumToVoteKey(voteInfoWidget->vote.dateofvote(), voteInfoWidget->vote.votenumber(), this->buffer);
	if (userChoise) {
		voteDoesNotExistInListAlready = this->addVoteToUserVoteListWidget(this->buffer, WGlobals::UserVoteList::YES);
	} else {
		voteDoesNotExistInListAlready = this->addVoteToUserVoteListWidget(this->buffer, WGlobals::UserVoteList::NO);
	}

	if (voteDoesNotExistInListAlready) {
		std::string voteId = this->addRepresentativesChoisesToVoteIdToChoisesMap(voteInfoWidget->vote);
		this->updateRepresentativeMatch(voteId);
		this->setRepresentativeMatchValuesToSeatMap();
	}
}

void VotesAnalyser::handleRepresentativeCliked(int seat)
{
	W::Log::info() << "Representative clicked, seat: " << seat;
	unsigned id = this->dataContainer.convertSeatToId(seat);
	const WGlobals::RepresentativeInfo & politician = this->dataContainer.findRepresentativeInfo(id);
	this->representativeInfo->show();
	this->representativeInfo->setShownRepresentative(politician);
}

void VotesAnalyser::handleShareButtonPressed(void)
{
	if (!this->userSession.isLoggedIn()) {
		showCreateAccountDialog();
		return ;
	}

	if (this->currentUserListInfo.listId == 0) {
		W::showUserErrorDialog(Wt::WString::tr("error.vaw.uservotelist-notsaved"));
		return ;
	}

	if (this->currentUserListInfo.set) {
		Wt::WString linkTextEncoded = this->createVoteListLink(this->userSession.getUserInfo().username(), this->currentUserListInfo.listId);
		Wt::WString linkText = this->createVoteListLink(this->userSession.getUserInfo().username(), this->currentUserListInfo.listId, false);
		showShareDialog(linkTextEncoded, linkText, Wt::WString::tr("share"), Wt::WApplication::instance()->locale());
	} else {
		W::showUserErrorDialog(Wt::WString::tr("error.vaw.uservotelist-notselected"));
	}
}

void VotesAnalyser::handleSaveButtonPressed(void)
{
	if (!this->userSession.isLoggedIn()) {
		showCreateAccountDialog();
		return ;
	}

	unsigned listId = 0;
	int index = this->ownedVotesLists->currentIndex();
	if (index != 0) {
		listId = this->ownedVotesIds[index-1];						//First index is the info text
		W::Log::debug() << "Updating old list with id " << listId;
	} else {
		listId = this->userSession.getNewUniqueUserListId();
		W::Log::debug() << "Creating new list with id " << listId;
	}

	Wt::WDialog dialog(Wt::WString::tr("dialog.save.title"));
	Wt::WText * listName = new Wt::WText(Wt::WString::tr("dialog.save.field.name"), dialog.contents());
	new Wt::WBreak(dialog.contents());
	Wt::WLineEdit editName(dialog.contents());
	new Wt::WBreak(dialog.contents());
	Wt::WText * listDescription = new Wt::WText(Wt::WString::tr("dialog.save.field.description"), dialog.contents());
	new Wt::WBreak(dialog.contents());
	Wt::WTextArea editDescription(dialog.contents());
	new Wt::WBreak(dialog.contents());
	Wt::WPushButton ok(Wt::WString::tr("dialog.save.button.ok"), dialog.contents());
	Wt::WPushButton cancel(Wt::WString::tr("dialog.save.button.cancel"), dialog.contents());

	listName->setStyleClass("vaw-dialog-save-title-info");
	editName.setStyleClass("vaw-dialog-save-title");
	editName.setMaxLength(WGlobals::USERVOTELIST_NAME_MAX_LENGTH);
	listDescription->setStyleClass("vaw-dialog-save-description-info");
	editDescription.setStyleClass("vaw-dialog-save-description");

	Wt::WApplication::instance()->globalEscapePressed().connect(&dialog, &Wt::WDialog::reject);
	ok.clicked().connect(&dialog, &Wt::WDialog::accept);
	cancel.clicked().connect(&dialog, &Wt::WDialog::reject);

	if (dialog.exec() == Wt::WDialog::Accepted) {
		const std::string name = editName.text().toUTF8();
		const std::string description = editDescription.text().toUTF8();
		if ((name.length() != 0) && (description.length() != 0)) {
			bool anySet = false;
			this->userVoteList->Clear();
			this->userVoteList->set_name(name);
			this->userVoteList->set_description(description);
			for (unsigned i = 0; i < (unsigned)interestedVotesTable->rowCount(); ++i) {
					InterestedVotesListElementWidget * element = (InterestedVotesListElementWidget *)this->interestedVotesTable->elementAt(i, 0)->widget(0);
					if (element->isHidden()) {
						continue;
					}

					WDbType::UserVoteList::UserVoteInfo * userVoteInfo = this->userVoteList->mutable_uservoteinfo()->Add();
					userVoteInfo->set_voteid(element->voteId());
					if (!element->isVoteSet()) {
						userVoteInfo->set_decision(WDbType::UserVoteList::NO_OPINION);
					} else {
						if (element->userChoise()) {
							userVoteInfo->set_decision(WDbType::UserVoteList::YES);
						} else {
							userVoteInfo->set_decision(WDbType::UserVoteList::NO);
						}
					}
					W::Log::debug() << "Added vote " << element->voteId() << " to owned list";
					anySet = true;
			}
			if (anySet) {
				this->userVoteList->set_relatedseasonstartingyear(this->currentUserListInfo.relatedSeasonStartingYear);
				this->userSession.saveUserVoteList(*this->userVoteList, listId);
				this->setFollowedListsList();
				this->setOwnedListsList();
				this->updateWidget();
				this->ownedVotesLists->setCurrentIndex(this->ownedVotesLists->count());
				this->currentUserListInfo.listId = listId;
			} else {
				W::showUserErrorDialog(Wt::WString::tr("error.vaw.match.list.empty"));
			}
		} else {
			W::showUserErrorDialog(Wt::WString::tr("error.vaw.match.name.empty"));
		}
	}
}

void VotesAnalyser::handleDeleteButtonPressed(void)
{
	if (!this->userSession.isLoggedIn()) {
		W::Log::error() << "handleOwnedListsListSelected called when not logged in";
		showCreateAccountDialog();
		return ;
	}

	unsigned index = this->ownedVotesLists->currentIndex();		//First index is the info text
	if (index == 0) {
		W::showUserErrorDialog(Wt::WString::tr("error.vaw.delete.empty"));
		return ;
	}

	index--;
	if ((unsigned)index >= this->ownedVotesIds.size()){
		W::Log::error() << "handleDeleteButtonPressed index " << index << " clicked even though list size was " << this->ownedVotesIds.size();
		return ;
	}

	unsigned listId = this->ownedVotesIds[index];
	W::Log::info() << "Deleting uservotelist with listId " << listId;
	this->userSession.deleteUserVoteList(listId);
	this->redrawWidget();
}

void VotesAnalyser::handleOwnedListsListSelected(int index)
{
	this->clearInterestedVotesTable();
	if (index == 0) {		//i.e. the info text text of first element
		this->currentUserListInfo.set = false;
		return ;
	}
	if (!this->userSession.isLoggedIn()) {
		W::Log::error() << "handleOwnedListsListSelected called when not logged in";
		return ;
	}
	--index;
	if ((unsigned)index >= this->ownedVotesIds.size()){
		W::Log::error() << "handleOwnedListsListSelected index " << index << " clicked even though list size was " << this->ownedVotesIds.size();
		return ;
	}

	W::Log::debug() << "handleOwnedListsListSelected: index " << index << " with list " << this->ownedVotesIds[index] << " selected";
	if (!setUserVoteList(this->userSession.getUserInfo().username(), this->ownedVotesIds[index], *this->userVoteList)) {
		W::showUserErrorDialog(Wt::WString::tr("dialog.error.uservotelist-not-found"));
		return ;
	}
	for (google::protobuf::RepeatedPtrField<WDbType::UserVoteList_UserVoteInfo>::const_iterator it = this->userVoteList->uservoteinfo().begin(); it != this->userVoteList->uservoteinfo().end(); ++it) {
		this->addVoteToUserVoteListWidget(it->voteid(), it->decision());
	}
	this->currentUserListInfo.setRelatedSeasonStartingYear(this->userVoteList->relatedseasonstartingyear(), this->ownedVotesIds[index]);
	this->followedMonthList->setCurrentIndex(index+1);		//+1 For info text at the zeroth index
}

void VotesAnalyser::handleFollowListsListSelected(int index)
{
	if (index == 0) {		//i.e. the info text text of first element
		return ;
	}
	if (!this->userSession.isLoggedIn()) {
		W::Log::error() << "handleOwnedListsListSelected called when not logged in";
		return ;
	}
	if ((unsigned)index < this->ownedVotesIds.size()) {						//Update also owned list
		this->handleOwnedListsListSelected(index);
		this->ownedVotesLists->setCurrentIndex(index);
		return ;
	}
	--index;				//First element is the info text
	if ((unsigned)index >= this->followListsIds.size()){
		W::Log::error() << "handleFollowListsListSelected index " << index << " clicked even though list size was " << this->followListsIds.size();
		return ;
	}
	W::Log::debug() << "handleFollowListsListSelected: index " << index << " with list " << this->followListsIds[index].listId << " selected";
	if (!setUserVoteList(this->followListsIds[index].owner, this->followListsIds[index].listId, *this->userVoteList)) {
		W::showUserErrorDialog(Wt::WString::tr("dialog.error.uservotelist-not-found"));
	}
}

void VotesAnalyser::handleActiveMonthsListSelected(int index)
{
	W::Log::info() << "Index in handleActiveMonthsListSelected " << index;
	if (index == 0) {														//First line is the description line
		return ;
	}
	--index;
	const WGlobals::PoliticalSeason & season = this->dataContainer.findPoliticalSeasonInfo(this->shownVotesState->getShownSeasonStartingYear());
	if (!season.IsInitialized() || !season.monthscontainingvotes_size()) {
		W::Log::error() << "Season was not initialized did not contain any active months: " << season.monthscontainingvotes_size();
		return ;
	}
	if (index >= season.monthscontainingvotes().size()) {
		W::Log::error() << "List index invalid in handleActiveMonthsListSelected";
		return ;
	}
	const WDbType::MonthYear & monthYear = season.monthscontainingvotes().Get(index);
	this->getVotesForMonth(monthYear.year(), monthYear.month());
}

void VotesAnalyser::handleVisualizationDone(void)
{
	this->isVisualizationOperationOngoing = false;
	W::Log::info() << "handleVisualizationDone";
}

void VotesAnalyser::handleRemoveVoteFromVotelistButtonPressed(const Wt::WText * votename)
{
	std::string voteId = votename->text().toUTF8();
	W::Log::debug() << "handleRemoveVoteFromVotelist " << voteId;

	for (unsigned i = 0; i < (unsigned)interestedVotesTable->rowCount(); ++i) {
		InterestedVotesListElementWidget * element = (InterestedVotesListElementWidget *)this->interestedVotesTable->elementAt(i, 0)->widget(0);
		if (element->isHidden()) {
			continue;
		}
		if (element->voteId() == voteId) {
			element->hide();
			element->resetDecision();
			W::Log::debug() << "Removed vote " << element->voteId();
			break;
		}
	}
	this->removeRepresentativesChoisesFromVoteIdToChoisesMap(voteId);
	this->resetcurrentRepresentativeMatchSum();
	this->updateRepresentativeMatch();
	this->setRepresentativeMatchValuesToSeatMap();
}

void VotesAnalyser::handleVoteFromInterestedTableClicked(const Wt::WText * votename)
{
	std::string voteId = votename->text().toUTF8();
	this->showOnlyOneVote(voteId);
}

void VotesAnalyser::handleUserChoiseChangedOnVote(std::string voteId)
{
	this->resetcurrentRepresentativeMatchSum();
	this->updateRepresentativeMatch();
	this->setRepresentativeMatchValuesToSeatMap();
}

void VotesAnalyser::handleShowMoreVotesButtonPressed(void)
{
	this->getVotesForMonth(this->shownVotesState->getShownYear(), this->shownVotesState->getShownMonth(), true);
}

void VotesAnalyser::handleVotesRightSideButtonPressed(void)
{
	W::Log::debug() << "handleVotesRightSideButtonPressed";

	const WGlobals::PoliticalSeason & season = this->dataContainer.findPoliticalSeasonInfo(this->shownVotesState->getShownSeasonStartingYear());
	if (!season.IsInitialized() || !season.monthscontainingvotes_size()) {
		W::Log::error() << "season was not initialized did not contain any active months: " << season.monthscontainingvotes_size();
		return ;
	}

	bool found = false;
	google::protobuf::RepeatedPtrField<WDbType::MonthYear>::const_iterator prev = season.monthscontainingvotes().end();
	for (google::protobuf::RepeatedPtrField<WDbType::MonthYear>::const_iterator it = season.monthscontainingvotes().begin(); it != season.monthscontainingvotes().end(); ++it) {
		if ((it->month() == this->shownVotesState->getShownMonth()) && (it->year() == this->shownVotesState->getShownYear())) {
			if (prev != season.monthscontainingvotes().end()) {
				//If previous month exists, set that as current
				this->shownVotesState->setVotesShownForMonth(prev->year(), prev->month(), this->shownVotesState->getShownSeasonStartingYear());
				this->getVotesForMonth(this->shownVotesState->getShownYear(), this->shownVotesState->getShownMonth());
				this->activeMonthList->setCurrentIndex(std::distance(season.monthscontainingvotes().begin(), it));
			} else {
				//Else indicate to user that there is no more votes
				W::showUserErrorDialog(Wt::WString::tr("vw.votelistarea.button.end"));
			}
			found = true;
			break;
		}
		prev = it;
	}
	if (!found) {
		W::Log::error() << "setted year not found";
	}
}

void VotesAnalyser::handleVotesLeftSideButtonPressed(void)
{
	W::Log::debug() << "handleVotesLeftSideButtonPressed";

	const WGlobals::PoliticalSeason & season = this->dataContainer.findPoliticalSeasonInfo(this->shownVotesState->getShownSeasonStartingYear());
	if (!season.IsInitialized() || !season.monthscontainingvotes_size()) {
		W::Log::error() << "season was not initialized did not contain any active months: " << season.monthscontainingvotes_size();
		return ;
	}

	bool found = false;
	for (google::protobuf::RepeatedPtrField<WDbType::MonthYear>::const_iterator it = season.monthscontainingvotes().begin(); it != season.monthscontainingvotes().end(); ++it) {
		if ((it->month() == this->shownVotesState->getShownMonth()) && (it->year() == this->shownVotesState->getShownYear())) {
			google::protobuf::RepeatedPtrField<WDbType::MonthYear>::const_iterator next = it;
			++next;
			if (next != season.monthscontainingvotes().end()) {
				//If next month exists, set it as current
				this->shownVotesState->setVotesShownForMonth(next->year(), next->month(), this->shownVotesState->getShownSeasonStartingYear());
				this->getVotesForMonth(this->shownVotesState->getShownYear(), this->shownVotesState->getShownMonth());
				this->activeMonthList->setCurrentIndex(std::distance(season.monthscontainingvotes().begin(), next) + 1);
			} else {
				//Else indicate to user that there is no more votes
				W::showUserErrorDialog(Wt::WString::tr("vw.votelistarea.button.end"));
			}
			found = true;
			break;
		}
	}
	if (!found) {
		W::Log::error() << "setted year not found";
	}
}

void VotesAnalyser::handleMatchButtonPressed(void)
{
	if (this->isVisualizationOperationOngoing) {
		W::Log::debug() << "Matching canceled - busy";
		return ;
	}
	this->setRepresentativeMatchValuesToSeatMap();
}

bool VotesAnalyser::initialize(void)
{
	if (this->activated) {
		return true;
	}
	this->doJavascriptInitialization();
	const WGlobals::PoliticalSeasonsList & seasonsList = this->dataContainer.findPoliticalSeasonsList();
	if (!seasonsList.IsInitialized()) {
		W::Log::error() << "seasonsList was not initialized";
		return false;
	}
	const WGlobals::PoliticalSeason & season = this->dataContainer.findPoliticalSeasonInfo(seasonsList.latestseason());
	if (!season.IsInitialized() || !season.monthscontainingvotes_size()) {
		W::Log::error() << "season was not initialized did not contain any active months: " << season.monthscontainingvotes_size();
		return false;
	}
	this->setPartySeatingInfoToSeatMap(seasonsList.latestseason());
	this->shownVotesState->setVotesShownForMonth((*(season.monthscontainingvotes().begin())).year(), (*(season.monthscontainingvotes().begin())).month(), seasonsList.latestseason());

	this->setActiveMonthsList(season);
	this->setFollowedListsList();
	this->setOwnedListsList();
	this->resetcurrentRepresentativeMatchSum();
	return true;
}

void VotesAnalyser::allocateMoreVoteListTableElements(const unsigned numberOfElementsToBeAllocated)
{
	if (this->votesTable->rowCount() != 0) {
		this->votesTable->deleteRow(this->votesTable->rowCount() - 1);
	}

	unsigned newSize = this->votesTable->rowCount() + numberOfElementsToBeAllocated;
	for (unsigned index = this->votesTable->rowCount(); index < newSize; ++index) {
		VotesTableElementWidget * element = new VotesTableElementWidget;
		element->clickedEvent().connect(this, &VotesAnalyser::handleVoteSelected);
		element->moveToInterestingButtonClicked().connect(this, &VotesAnalyser::handleMoveToInterestingButtonPresed);

		this->votesTable->elementAt(index, 0)->addWidget(element);
		this->votesTable->elementAt(index, 0)->hide();
	}
	Wt::WText * showMoreButton = new Wt::WText(Wt::WString::tr("vw.button.show-more-votes"));
	showMoreButton->clicked().connect(this, &VotesAnalyser::handleShowMoreVotesButtonPressed);
	showMoreButton->setStyleClass("vaw-showMoreButton");
	this->votesTable->elementAt(this->votesTable->rowCount(), 0)->addWidget(showMoreButton);
	this->votesTable->elementAt(this->votesTable->rowCount() - 1, 0)->hide();
}

void VotesAnalyser::createVotesListTables(void)
{
	this->allocateMoreVoteListTableElements(DEFAULT_SIZE_OF_VOTES_SHOWN_IN_LIST);

	for (unsigned  i = 0; i < DEFAULT_NUMBER_OF_INTERESTED_VOTES_SHOWN_IN_LIST; ++i) {
		InterestedVotesListElementWidget * element = new InterestedVotesListElementWidget;
		element->clickedEvent().connect(this, &VotesAnalyser::handleVoteFromInterestedTableClicked);
		element->deletedEvent().connect(this, &VotesAnalyser::handleRemoveVoteFromVotelistButtonPressed);
		element->userChoiseChangedOnVote().connect(this, &VotesAnalyser::handleUserChoiseChangedOnVote);
		element->hide();
		this->interestedVotesTable->elementAt(i, 0)->addWidget(element);
	}
}

void VotesAnalyser::getVotesForMonth(const unsigned year, const unsigned & month, bool skipAlreadyShowVotes)
{
	bool showMoreVotesButtonVisible = false;
	bool isSomethingNewShown = false;
	unsigned index = 0;
	const WGlobals::ListOfVotesForMonth & montList = this->dataContainer.findVotesForMonth(year, month);

	if (!montList.IsInitialized()) {
		W::Log::warn() << "Did not find votes for " << month << "." << year;
		return ;
	}

	for (google::protobuf::RepeatedPtrField<std::string>::const_iterator it = montList.voteid().begin(); it != montList.voteid().end(); ++it) {
		if (skipAlreadyShowVotes && ((index + 1) < (unsigned)this->votesTable->rowCount()) && !this->votesTable->elementAt(index, 0)->isHidden()) {
			++index;
			continue;
		}

		if (isSomethingNewShown && (index % DEFAULT_SIZE_OF_VOTES_SHOWN_IN_LIST == 0)) {
			showMoreVotesButtonVisible = true;
			break;
		}
		const WGlobals::VoteInformation & vote = this->dataContainer.findVote(*it);
		if (!vote.IsInitialized()) {
			W::Log::warn() << "Did not find vote " << *it;
			break;
		}
		if (index == ((unsigned)this->votesTable->rowCount() - 1)) {
			W::Log::info() << "Allocating more memory to votesTable, current size " << this->votesTable->rowCount();
			this->allocateMoreVoteListTableElements(DEFAULT_SIZE_OF_VOTES_SHOWN_IN_LIST);
		}
		((VotesTableElementWidget *)this->votesTable->elementAt(index, 0)->widget(0))->setVote(vote, *it, this->shownVotesState->getShownSeasonStartingYear());
		this->votesTable->elementAt(index, 0)->show();
		++index;
		isSomethingNewShown = true;
	}
	for (; index < (unsigned)this->votesTable->rowCount(); ++index) {
		this->votesTable->elementAt(index, 0)->hide();
	}

	if (showMoreVotesButtonVisible) {
		this->votesTable->elementAt(this->votesTable->rowCount() - 1, 0)->show();
	}
	this->shownVotesState->setVotesShownForMonth(year, month, this->shownVotesState->getShownSeasonStartingYear());
	this->updateWidget();
}

void VotesAnalyser::setPoliticalPartySeatingToBuffer(std::string & buffer, const WGlobals::PoliticalSeasonSeating & seating)
{
	buffer.resize(seating.representativeseat_size());
	buffer[0] = 48;											//Chair does not belong to any party
	for (google::protobuf::RepeatedPtrField<WDbType::RepresentativeSeatingInfo>::const_iterator it = seating.representativeseat().begin(); it != seating.representativeseat().end(); ++it) {
		buffer[this->dataContainer.convertIdToSeat(it->representativekey())-1] = 48 + it->partyid();	//-1 for converting seat to index. Replace? move to client?
		//W::Log::info() << it->representativekey() << " - " << it->seat() << " " << this->dataContainer.convertIdToSeat(it->representativekey())-1 << " - "<< it->partyid();
	}
}

void VotesAnalyser::setPoliticalPartyNamesToBuffer(std::string & buffer, const WGlobals::PoliticalPartyNames & names)
{
	for (google::protobuf::RepeatedPtrField<WDbType::PoliticalPartyNames_PoliticalPartyName>::const_iterator it = names.party().begin();
			it != names.party().end();++ it) {
		std::string temp;
		if (WGlobals::convertUIntToStr((unsigned)it->party(), temp)) {
			buffer.append("({name:\"" + it->name() + "\", id:\"" + temp + "\"}),");
		} else {
			W::Log::warn() << "Could not convert partyId to UInt in setPoliticalPartyNamesToBuffer, id: " << it->party();
		}
	}
}

void VotesAnalyser::setRepresentativeChoisesToBuffer(std::string & buffer, const WGlobals::VoteInformation & vote)
{
	buffer.clear();
	buffer.resize(vote.voteinfo().size() + this->dataContainer.numberOfAdditionalSeatsInSeason() + 1, '2');																										//199 voters + chair
	W::Log::info() << "size: " << vote.voteinfo().size() << " (" << this->dataContainer.numberOfAdditionalSeatsInSeason() << ") chair " << vote.idofchair();
	for (google::protobuf::RepeatedPtrField<WDbType::RepresentativeVoteInfo>::const_iterator it = vote.voteinfo().begin(); it != vote.voteinfo().end(); ++it) {
		if (it->votedecision() == WDbType::VoteChoise::AWAY) {
			buffer[this->dataContainer.convertIdToSeat(it->id())-1] = '2';			//-1 for converting seat to index. Replace? move to client?
		} else if (it->votedecision() == WDbType::VoteChoise::YES) {
			buffer[this->dataContainer.convertIdToSeat(it->id())-1] = '1';
		} else if (it->votedecision() == WDbType::VoteChoise::NO) {
			buffer[this->dataContainer.convertIdToSeat(it->id())-1] = '0';
		} else if (it->votedecision() == WDbType::VoteChoise::EMPTY) {
			buffer[this->dataContainer.convertIdToSeat(it->id())-1] = '3';
		}
		//W::Log::info() << it->id() << " - " << this->dataContainer.convertIdToSeat(it->id()) << " - " << it->votedecision();
	}

	buffer[this->dataContainer.convertIdToSeat(vote.idofchair())-1] = '-';
}

void VotesAnalyser::setUserVoteList(const WGlobals::UserVoteList & userVoteList)
{
	this->clearInterestedVotesTable();
	this->voteListDescriptionArea->setText(Wt::WString::fromUTF8(userVoteList.description()));
	this->voteListDescriptionArea->show();
	this->resetcurrentRepresentativeMatchSum();

	unsigned index = 0;
	for (google::protobuf::RepeatedPtrField<WDbType::UserVoteList_UserVoteInfo>::const_iterator it = userVoteList.uservoteinfo().begin(); it != userVoteList.uservoteinfo().end(); ++it) {
		const WGlobals::VoteInformation & vote = this->dataContainer.findVote(it->voteid());
		if (!vote.IsInitialized()) {
			W::Log::warn() << "Did not find vote " << it->voteid();
			break;
		}

		this->addRepresentativesChoisesToVoteIdToChoisesMap(vote);
		if (it->decision() == WGlobals::UserVoteList::NO_OPINION) {
			((VotesTableElementWidget *)this->votesTable->elementAt(index, 0)->widget(0))->setVote(vote, it->voteid(), this->shownVotesState->getShownSeasonStartingYear());
		} else {
			((VotesTableElementWidget *)this->votesTable->elementAt(index, 0)->widget(0))->setVote(vote, it->voteid(), this->shownVotesState->getShownSeasonStartingYear(), it->decision() == WGlobals::UserVoteList::YES);
			this->updateRepresentativeMatch(it->voteid(), it->decision());
		}
		this->votesTable->elementAt(index, 0)->show();
		++index;
	}
	for (; index < (unsigned)this->votesTable->rowCount(); ++index) {
		this->votesTable->elementAt(index, 0)->hide();
	}
	this->setRepresentativeMatchValuesToSeatMap();
}

bool VotesAnalyser::setUserVoteList(const std::string & username, const unsigned listId, WGlobals::UserVoteList & userVoteList)
{
	if (!this->userSession.getUserVoteList(username, listId, userVoteList)) {
		W::Log::info() << "setUserVoteList: list given as param not found, username: " << username << " list id: " << listId;
		return false;
	}
	this->dataContainer.findPoliticalSeasonInfo(userVoteList.relatedseasonstartingyear());
	this->setUserVoteList(userVoteList);
	this->shownVotesState->setVotesShownCustomList(username, listId);
	this->updateWidget();
	return true;
}

Wt::WString VotesAnalyser::createVoteListLink(const std::string & user, unsigned listId, bool encoded)
{
	return Wt::WString("http://{1}/{2}?{3}={4}{5}{6}={7}").arg(
			Wt::WApplication::instance()->environment().hostName()).arg(
					Wt::WApplication::instance()->bookmarkUrl()).arg("user").arg(user).arg(encoded ? "&amp;" : "&").arg("list").arg((int)listId);
}

bool VotesAnalyser::showOnlyOneVote(const std::string & voteId)
{
	W::Log::debug() << "showOnlyOneVote: vote: " << voteId;
	const WGlobals::VoteInformation & vote = this->dataContainer.findVote(voteId);
	if (vote.IsInitialized()) {
		//Because list can consist only of votes from same season, the strating year the same
		((VotesTableElementWidget *)this->votesTable->elementAt(0, 0)->widget(0))->setVote(vote, voteId, this->shownVotesState->getShownSeasonStartingYear());
		this->votesTable->elementAt(0, 0)->show();
		for (unsigned index = 1; index < (unsigned)this->votesTable->rowCount(); ++index) {
			this->votesTable->elementAt(index, 0)->hide();
		}
		setSetVoteRecordInformation(vote.dateofvote(), vote.votenumber());//Commented because source sucks
		this->setRepresentativeChoisesToBuffer(this->buffer, vote);
		this->isVisualizationOperationOngoing = true;
		this->doJavaScript("visualizeVoteChoisesWithMap(\"" + this->buffer + "\");");
	} else {
		W::Log::warn() << "Did not find vote " << voteId << " in handleVoteFromInterestedTableClicked";
		return false;
	}

	const WGlobals::VoteStatistics & stats = this->dataContainer.findVoteStatistics(voteId);
	if (stats.IsInitialized()) {
		this->buffer.clear();
		this->setVoteStatisticsToBuffer(stats);
		this->doJavaScript("drawVoteStatistics(" + this->buffer + ");");
	}
	this->updateWidget();
	return true;
}

std::string VotesAnalyser::addRepresentativesChoisesToVoteIdToChoisesMap(const WGlobals::VoteInformation & vote)
{
	std::string voteId;
	WDb::convertDateAndVoteNumToVoteKey(vote.dateofvote(), vote.votenumber(), voteId);
	std::vector<WDbType::VoteChoise> representativeChoises(vote.voteinfo().size() + this->dataContainer.numberOfAdditionalSeatsInSeason(), WDbType::VoteChoise::AWAY);

	for (google::protobuf::RepeatedPtrField<WDbType::RepresentativeVoteInfo>::const_iterator it = vote.voteinfo().begin(); it != vote.voteinfo().end(); ++it) {
		representativeChoises[this->dataContainer.convertIdToSeat(it->id())-1] = it->votedecision(); //-1 for converting seat to index. Replace? move to client?
		//W::Log::info() << it->id() << " - " << this->dataContainer.convertIdToSeat(it->id()) << " - " << it->votedecision();
	}
	this->mapVoteIdToRepresentativeChoicesVector[voteId] = representativeChoises;
	W::Log::debug() << "Added vote " << voteId << " to mapVoteIdToRepresentativeChoicesVector. Currently it has " <<
			mapVoteIdToRepresentativeChoicesVector.size() << " votes in it";
	return voteId;
}

void VotesAnalyser::removeRepresentativesChoisesFromVoteIdToChoisesMap(const std::string & voteId)
{
	std::map<std::string, std::vector<WDbType::VoteChoise> >::const_iterator it;
	if ((it = this->mapVoteIdToRepresentativeChoicesVector.find(voteId)) == this->mapVoteIdToRepresentativeChoicesVector.end()) {
		W::Log::error() << "Tried to remove vote " << voteId << " from mapVoteIdToRepresentativeChoicesVector unsuccessfully";
	}
	this->mapVoteIdToRepresentativeChoicesVector.erase(voteId);
	W::Log::debug() << "Removed vote " << voteId << " from mapVoteIdToRepresentativeChoicesVector. Current size: " << this->mapVoteIdToRepresentativeChoicesVector.size();
}

void VotesAnalyser::updateRepresentativeMatch(void)
{
	unsigned numberOfVotesInUserVoteList = 0;
	this->resetcurrentRepresentativeMatchSum();
	if (this->mapVoteIdToRepresentativeChoicesVector.size() == 0) {
		W::Log::warn() << "mapVoteIdToRepresentativeChoicesVector.size() == 0";
		return ;
	}

	if (this->currentRepresentativeMatchSum.size() != this->mapVoteIdToRepresentativeChoicesVector.begin()->second.size()) {
		W::Log::warn() << "currentRepresentativeMatchSum.size() != mapVoteIdToRepresentativeChoicesVector.begin()->second.size()";
		return ;
	}

	for (unsigned i = 0; i < (unsigned)interestedVotesTable->rowCount(); ++i) {
		InterestedVotesListElementWidget * element = (InterestedVotesListElementWidget *)this->interestedVotesTable->elementAt(i, 0)->widget(0);
		if (element->isHidden()) {
			continue;
		}
		if (!element->isVoteSet()) {
			W::Log::warn() << "Found vote without user choice in updateRepresentativeMatch, ignoring";
			continue;
		}
		std::map<std::string, std::vector<WDbType::VoteChoise> >::const_iterator it;
		if ((it = this->mapVoteIdToRepresentativeChoicesVector.find(element->voteId())) == this->mapVoteIdToRepresentativeChoicesVector.end()) {
			W::Log::warn() << "Did not find voteId " << element->voteId() << " from mapVoteIdToRepresentativeChoicesVector in updateRepresentativeMatch";
			continue;
		}

		for (std::vector<WDbType::VoteChoise>::const_iterator c = it->second.begin(); c != it->second.end(); ++c) {
			unsigned index = std::distance(std::vector<WDbType::VoteChoise>::const_iterator(it->second.begin()), c);
			if (*c != WDbType::VoteChoise::AWAY) {
				WDbType::VoteChoise u;
				if (element->userChoise()) {
					u = WDbType::VoteChoise::YES;
				} else {
					u = WDbType::VoteChoise::NO;
				}
				this->currentRepresentativeMatchSum[index].first += (int)(*c == u);
				++this->currentRepresentativeMatchSum[index].second;
			}
		}
		++numberOfVotesInUserVoteList;
	}
}

void VotesAnalyser::updateRepresentativeMatch(const std::string & voteId)
{
	if (this->mapVoteIdToRepresentativeChoicesVector.size() == 0) {
		W::Log::warn() << "mapVoteIdToRepresentativeChoicesVector.size() == 0";
		return ;
	}

	if (this->currentRepresentativeMatchSum.size() != this->mapVoteIdToRepresentativeChoicesVector.begin()->second.size()) {
		W::Log::warn() << "currentRepresentativeMatchSum.size() != mapVoteIdToRepresentativeChoicesVector.begin()->second.size()";
		return ;
	}

	for (unsigned i = 0; i < (unsigned)interestedVotesTable->rowCount(); ++i) {
		InterestedVotesListElementWidget * element = (InterestedVotesListElementWidget *)this->interestedVotesTable->elementAt(i, 0)->widget(0);
		if (element->isHidden()) {
			continue;
		}
		if (!element->isVoteSet()) {
			W::Log::warn() << "Found vote without user choice in updateRepresentativeMatch, ignoring";
			continue;
		}
		if (element->isEqualVote(voteId)) {
			std::map<std::string, std::vector<WDbType::VoteChoise> >::const_iterator it;
			if ((it = this->mapVoteIdToRepresentativeChoicesVector.find(element->voteId())) == this->mapVoteIdToRepresentativeChoicesVector.end()) {
				W::Log::warn() << "Did not find voteId " << element->voteId() << " from mapVoteIdToRepresentativeChoicesVector in updateRepresentativeMatch";
				continue;
			}

			for (std::vector<WDbType::VoteChoise>::const_iterator c = it->second.begin(); c != it->second.end(); ++c) {
				unsigned index = std::distance(std::vector<WDbType::VoteChoise>::const_iterator(it->second.begin()), c);
				if ((*c != WDbType::VoteChoise::AWAY) && (*c != WDbType::VoteChoise::EMPTY)) {
					WDbType::VoteChoise u;
					if (element->userChoise()) {
						u = WDbType::VoteChoise::YES;
					} else {
						u = WDbType::VoteChoise::NO;
					}
					this->currentRepresentativeMatchSum[index].first += (int)(*c == u);
					++this->currentRepresentativeMatchSum[index].second;
				}
			}
			W::Log::debug() << "Updated vote " << voteId <<  " in updateRepresentativeMatch";
			return ;
		}
	}
	W::Log::error() << "Tried to update vote " << voteId <<  " in updateRepresentativeMatch but it was not found";
}

void VotesAnalyser::updateRepresentativeMatch(const std::string & voteId, WDbType::UserVoteList::Vote userChoise)
{
	if (this->mapVoteIdToRepresentativeChoicesVector.size() == 0) {
		W::Log::warn() << "mapVoteIdToRepresentativeChoicesVector.size() == 0";
		return ;
	}

	if (this->currentRepresentativeMatchSum.size() != this->mapVoteIdToRepresentativeChoicesVector.begin()->second.size()) {
		W::Log::warn() << "currentRepresentativeMatchSum.size() != mapVoteIdToRepresentativeChoicesVector.begin()->second.size()";
		return ;
	}

	std::map<std::string, std::vector<WDbType::VoteChoise> >::const_iterator it;
	if ((it = this->mapVoteIdToRepresentativeChoicesVector.find(voteId)) == this->mapVoteIdToRepresentativeChoicesVector.end()) {
		W::Log::warn() << "Did not find voteId " << voteId << " from mapVoteIdToRepresentativeChoicesVector in updateRepresentativeMatch";
		return ;
	}

	for (std::vector<WDbType::VoteChoise>::const_iterator c = it->second.begin(); c != it->second.end(); ++c) {
		unsigned index = std::distance(std::vector<WDbType::VoteChoise>::const_iterator(it->second.begin()), c);
		if (*c != WDbType::VoteChoise::AWAY) {
			WDbType::UserVoteList::Vote v = WDbType::UserVoteList::NO;
			if (*c == WDbType::VoteChoise::YES) {
				v = WDbType::UserVoteList::YES;
			}
			this->currentRepresentativeMatchSum[index].first += (int)(v == userChoise);
			++this->currentRepresentativeMatchSum[index].second;
		}
	}
	W::Log::debug() << "Updated vote " << voteId <<  " in updateRepresentativeMatch";
	return ;
}

void VotesAnalyser::setRepresentativeMatchValuesToSeatMap(void)
{
	if (this->isVisualizationOperationOngoing) {
		W::Log::warn() << "Visualization ongoing in setRepresentativeMatchValuesToSeatMap";
		return ;
	}

	this->isVisualizationOperationOngoing = true;
	buffer.clear();
	static const unsigned sizeOfBuffer = 32;
	char value[sizeOfBuffer];
	for (std::vector<std::pair<unsigned, unsigned> >::const_iterator it = this->currentRepresentativeMatchSum.begin(); it != this->currentRepresentativeMatchSum.end(); ++it) {
		memset(value, '\0', sizeOfBuffer);
		if (it->second == 0) {
			sprintf(value, "%u-%u;", 0, 0);
		} else {
			float match = (((float)it->first)/((float)it->second)) * 100.f;
			sprintf(value, "%u-%u;", (int)match, it->second);
		}
		buffer.append(value);
	}
	this->doJavaScript("visualizeMatchWithMap(\"" + this->buffer + "\");");
}

void VotesAnalyser::resetcurrentRepresentativeMatchSum(void)
{
	//-1 for chair who does not participate
	this->currentRepresentativeMatchSum.assign(200 + this->dataContainer.numberOfAdditionalSeatsInSeason() - 1, std::make_pair<unsigned, unsigned>(0, 0));//FIXME: hardcoded
}

void VotesAnalyser::setJsLocaleDictionary(void)
{
	this->buffer.clear();
	this->buffer.append("setLocaleDictionary({");

	this->buffer.append("matchStart: \"" + Wt::WString::tr("vw.js.match-line.start").toUTF8() + "\",");
	this->buffer.append("matchMiddle: \"" + Wt::WString::tr("vw.js.match-line.middle").toUTF8() + "\",");
	this->buffer.append("matchEnd: \"" + Wt::WString::tr("vw.js.match-line.end").toUTF8() + "\",");
	this->buffer.append("yes: \"" + Wt::WString::tr("vw.vote.yes").toUTF8() + "\",");
	this->buffer.append("no: \"" + Wt::WString::tr("vw.vote.no").toUTF8() + "\",");
	this->buffer.append("empty: \"" + Wt::WString::tr("vw.vote.empty").toUTF8() + "\",");
	this->buffer.append("away: \"" + Wt::WString::tr("vw.vote.away").toUTF8() + "\",");
	this->buffer.append("vote: \"" + Wt::WString::tr("vw.js.votes").toUTF8() + "\",");

	this->buffer.append("});");
	this->doJavaScript(this->buffer);
}

void VotesAnalyser::setSelectedListDescription(const unsigned month, const unsigned year)
{
	this->selectedListDescription->setText(Wt::WString("{1}<br />{2}").arg(WGlobals::convertUnsignedMonthToStr(month)).arg((int)year));
}

void VotesAnalyser::setSelectedListDescription(const std::string & username, const std::string listName)
{
	this->selectedListDescription->setText(Wt::WString("{1}<br />{2}").arg(username).arg(listName));
}

void VotesAnalyser::setVoteStatisticsToBuffer(const WDbType::Dictionary & stats)
{
	this->buffer.append("[");
	for (::google::protobuf::RepeatedPtrField< ::WDbType::Pair >::const_iterator it = stats.pairs().begin();
			it != stats.pairs().end(); ++it) {
		this->buffer.append(it->value() + ",");
	}
	this->buffer.append("]");
#ifdef DEBUG
	W::Log::debug() << "setVoteStatisticsToBuffer: " << this->buffer;
#endif
}

void VotesAnalyser::doJavascriptInitialization(void)
{
	if (this->activated) {
		return;
	}
	this->doJavaScript("initVotesAnalyser(\"" + this->id() + "\");");

	const WGlobals::PoliticalPartyNames & partyNames = this->dataContainer.findPoliticalPartyNameInfo();
	if (!partyNames.IsInitialized()) {
		W::Log::error() << "partyNames was not initialized";
		return ;
	}
	this->buffer.clear();
	this->buffer.append("setPartNames([");
	this->setPoliticalPartyNamesToBuffer(this->buffer, partyNames);
	this->buffer.append("])");
	this->doJavaScript(this->buffer);
	this->activated = true;
	this->setJsLocaleDictionary();
}

void VotesAnalyser::setPartySeatingInfoToSeatMap(const unsigned & seasonStartingYear)
{
	const WGlobals::PoliticalSeasonSeating & seating = this->dataContainer.findSeatingForSeason(seasonStartingYear);
	if (!seating.IsInitialized()) {
		W::Log::error() << "partyNames was not initialized";
		return ;
	}
	this->buffer.clear();
	this->setPoliticalPartySeatingToBuffer(this->buffer, seating);//, this->buffer.length() + 1);
	this->doJavaScript("setPartyInfo(\"" + this->buffer + "\");");

	this->buffer.clear();
	if (WGlobals::convertIntToStr(seating.seatchange_size(), this->buffer)) {
		this->doJavaScript("setAdditionalSeats(" + this->buffer + ", \"" + Wt::WString::tr("vw.js.additional-seats").toUTF8() + "\");");
	} else {
		W::Log::warn() << "setAdditionalSeats was not called bacause convert to uint failed";
	}
}

void VotesAnalyser::updateWidget(void)
{
	if (this->shownVotesState->shouldMoveButtonBeShown() && this->votesRightSideButton->isHidden()) {		//Hidden state should be sync between buttons, use either to check it
		this->votesRightSideButton->setHidden(false);
		this->votesLeftSideButton->setHidden(false);
		this->voteListDescriptionArea->hide();
	} else if (!this->shownVotesState->shouldMoveButtonBeShown() && !this->votesRightSideButton->isHidden()) {
		this->votesRightSideButton->setHidden(true);
		this->votesLeftSideButton->setHidden(true);
	}

	//Above code will only change state when buttons state needs to be changed. Text, however, should be changed everytime
	if (this->shownVotesState->shouldMoveButtonBeShown()) {
		this->setSelectedListDescription(this->shownVotesState->getShownMonth(), this->shownVotesState->getShownYear());
	} else {
		if (this->userVoteList->IsInitialized()) {
			this->setSelectedListDescription(this->shownVotesState->getShowCustomListName().owner, this->userVoteList->name());
		}
	}

	if (this->shownVotesState->showAllVotes()) {
		this->followedMonthList->setCurrentIndex(0);
	} else {
		this->activeMonthList->setCurrentIndex(0);
	}
}

bool VotesAnalyser::addVoteToUserVoteListWidget(const std::string & voteId, WGlobals::UserVoteList::Vote decision)
{
	unsigned numberOfElementsShown = 0;
	for (unsigned i = 0; i < (unsigned)interestedVotesTable->rowCount(); ++i) {
		InterestedVotesListElementWidget * element = (InterestedVotesListElementWidget *)this->interestedVotesTable->elementAt(i, 0)->widget(0);
		if (element->isHidden()) {
			break;
		}
		if (element->isEqualVote(voteId)) {
			W::Log::debug() << "Vote already in interested list: " << voteId;
			return false;
		}
		++numberOfElementsShown;
	}

	W::Log::debug() << "Moving vote to interested: " << voteId;
	if (numberOfElementsShown == (unsigned)this->interestedVotesTable->rowCount()) {
		W::Log::debug() << "Allocating more memory to interestedVotesTable";
		InterestedVotesListElementWidget * element = new InterestedVotesListElementWidget;
		element->clickedEvent().connect(this, &VotesAnalyser::handleVoteFromInterestedTableClicked);
		element->deletedEvent().connect(this, &VotesAnalyser::handleRemoveVoteFromVotelistButtonPressed);
		element->userChoiseChangedOnVote().connect(this, &VotesAnalyser::handleUserChoiseChangedOnVote);
		element->hide();
		this->interestedVotesTable->elementAt(this->interestedVotesTable->rowCount() + 1, 0)->addWidget(element);
	}

	W::Log::info() << "numberOfShownElements " << numberOfElementsShown << " " << this->interestedVotesTable->rowCount();
	InterestedVotesListElementWidget * element = (InterestedVotesListElementWidget *)this->interestedVotesTable->elementAt(numberOfElementsShown, 0)->widget(0);
	element->setVoteId(voteId);
	if (decision == WGlobals::UserVoteList::YES) {
		element->setDecision(true);
	} else if (decision == WGlobals::UserVoteList::NO) {
		element->setDecision(false);
	}
	element->show();
	return true;
}

void VotesAnalyser::clearInterestedVotesTable(void)
{
	for (unsigned i = 0; i < (unsigned)interestedVotesTable->rowCount(); ++i) {
		InterestedVotesListElementWidget * element = (InterestedVotesListElementWidget *)this->interestedVotesTable->elementAt(i, 0)->widget(0);
		if (element->isHidden()) {
			continue;
		}
		element->hide();
		element->resetDecision();
	}
	this->resetcurrentRepresentativeMatchSum();
}

void VotesAnalyser::setSetVoteRecordInformation(const std::string & dateOfVote, const int voteNumber)
{
	const WGlobals::VoteRecord record = this->dataContainer.findVoteRecord(dateOfVote);
	if (record.IsInitialized()) {
		int index = voteNumber - 1;														//Convert to index
		if (index < record.votedescription_size()) {
			this->voteDescriptionArea->setText(Wt::WString::fromUTF8(record.votedescription(index)));
			this->voteDescriptionArea->show();
		} else {
			this->voteDescriptionArea->hide();
			W::Log::warn() << "Did not find index " << index << " in record for date: " << dateOfVote;
			this->voteDescriptionArea->setText(Wt::WString::tr("vw.voterecord.no-data"));
			this->voteDescriptionArea->show();
		}
		W::Log::debug() << "Number of infos in record " << record.votedescription().size() << " requested index " << index;
	} else {
		W::Log::warn() << "Did not find record for date: " << dateOfVote;
	}
}

void VotesAnalyser::setOwnedListsList(void)
{
	this->ownedVotesLists->clear();
	this->ownedVotesIds.clear();
	if (!this->userSession.isLoggedIn()) {
		this->ownedVotesLists->addItem(Wt::WString::tr("vw.ownedvotelist.default"));
		return ;
	}
	const WGlobals::UserInformation & userInfo = this->userSession.getUserInfo();
	this->ownedVotesLists->addItem(Wt::WString::tr("vw.ownedvotelist.create"));
	for (google::protobuf::RepeatedField<google::protobuf::uint32>::const_iterator it = userInfo.owneduserlists().begin(); it != userInfo.owneduserlists().end(); ++it) {
		if (this->userSession.getUserVoteList(this->userSession.getUserInfo().username(), *it, *this->userVoteList)) {
			this->ownedVotesLists->addItem(Wt::WString("{1}").arg(Wt::WString::fromUTF8(this->userVoteList->name())));
			this->ownedVotesIds.push_back(*it);
		} else {
			W::Log::error() << "Did not find userlist " << *it << " for user " << this->userSession.getUserInfo().username();
		}
	}
}

void VotesAnalyser::setFollowedListsList(void)
{
	this->followedMonthList->clear();
	this->followListsIds.clear();
	const WGlobals::UserInformation & userInfo = this->userSession.getUserInfo();

	if (!this->userSession.isLoggedIn()) {
		this->followedMonthList->addItem(Wt::WString::tr("vw.followdvotelist.login"));
		return ;
	}

	this->followedMonthList->addItem(Wt::WString::tr("vw.followdvotelist.default"));
	for (google::protobuf::RepeatedField<google::protobuf::uint32 >::const_iterator it = userInfo.owneduserlists().begin(); it != userInfo.owneduserlists().end(); ++it) {
		if (this->userSession.getUserVoteList(this->userSession.getUserInfo().username(), *it, *this->userVoteList)) {
			this->followedMonthList->addItem(Wt::WString("{1}").arg(Wt::WString::fromUTF8(this->userVoteList->name())));
			this->followListsIds.push_back(UserVoteListId(this->userSession.getUserInfo().username(), *it));
		} else {
			W::Log::error() << "Did not find userlist " << *it << " for user " << this->userSession.getUserInfo().username();
		}
	}

	for (google::protobuf::RepeatedPtrField<WDbType::UserInfo_userVoteListKey>::const_iterator it = userInfo.followeduserlists().begin(); it != userInfo.followeduserlists().end(); ++it) {
		if (this->userSession.getUserVoteList(it->username(), it->listid(), *this->userVoteList)) {
			this->followedMonthList->addItem(Wt::WString("{1}: {2}").arg(Wt::WString::fromUTF8(it->username())).arg(Wt::WString::fromUTF8(this->userVoteList->name())));
			this->followListsIds.push_back(UserVoteListId(it->username(), (int)it->listid()));
		} else {
			W::Log::error() << "Did not find userlist " << it->listid() << " for user " << it->username();
		}
	}
}

void VotesAnalyser::setActiveMonthsList(const WGlobals::PoliticalSeason & season)
{
	this->activeMonthList->clear();
	this->activeMonthList->addItem(Wt::WString::tr("vw.activemonthlist.default"));
	for (google::protobuf::RepeatedPtrField<WDbType::MonthYear>::const_iterator it = season.monthscontainingvotes().begin(); it != season.monthscontainingvotes().end(); ++it) {
		this->activeMonthList->addItem(Wt::WString("{1} - {2}").arg(WGlobals::convertUnsignedMonthToStr(it->month())).arg((int)it->year()));
	}
}
