#ifndef VOTES_ANALYSER_H_
#define VOTES_ANALYSER_H_

#include "Globals.hpp"
#include "db/VoteInfo.pb.h"

#include <Wt/WContainerWidget>
#include <Wt/WSignal>
#include <vector>
#include <string>

namespace Wt
{
	class WTemplate;
	class WTable;
	class WText;
	class WComboBox;
	class WPushButton;
}

namespace W
{
	static const unsigned DEFAULT_SIZE_OF_VOTES_SHOWN_IN_LIST = 50;
	static const unsigned DEFAULT_NUMBER_OF_INTERESTED_VOTES_SHOWN_IN_LIST = 20;
	static const size_t DEFAULT_BUFFER_SIZE = 2*1024;

	class DatabaseSession;
	class DataCache;
	class VotesVisualizerWidget;
	class RepresentativeInfoWidget;
	class VotesTableElementWidget;
	class ContentManager;
	class ShownVotesStateContainer;
	class UserSession;

	struct UserVoteListId
	{
		UserVoteListId(void){this->owner.reserve(WGlobals::USERNAME_MAX_LENGTH);}
		UserVoteListId(const std::string & username, const unsigned listId)
			: listId(listId), owner(username) {}
		unsigned listId;
		std::string owner;
	};

	struct CurrentUserListInfo
	{
		CurrentUserListInfo(void) : set(false), listId(0) {}
		bool set;
		unsigned relatedSeasonStartingYear;
		unsigned listId;

		void setRelatedSeasonStartingYear(const unsigned & year) {
			this->relatedSeasonStartingYear = year;
			this->listId = 0;
			this->set = true;
		}

		void setRelatedSeasonStartingYear(const unsigned & year, const unsigned listId) {
			this->relatedSeasonStartingYear = year;
			this->listId = listId;
			this->set = true;
		}
	};

	class VotesAnalyser : public Wt::WContainerWidget
	{
		public:
			VotesAnalyser(DataCache & session, UserSession & userSession, ContentManager & contentManager);
			~VotesAnalyser(void);
			void setActivated(void);
			void setActivated(const std::string & username, const unsigned listId);
			void setActivated(const std::string & voteId);
			void handleAuthStateChanged();
			void redrawWidget();
			inline bool isActivated(void) {return this->activated;}
			Wt::JSignal<int> & representativeCliked() {return representativeClikedSignal;}
			Wt::JSignal<void> & visualizationCompleted() {return visualizationCompletedSignal;}

		private:
			bool activated;
			bool isVisualizationOperationOngoing;

			UserSession & userSession;

			Wt::JSignal<int> representativeClikedSignal;
			Wt::JSignal<void> visualizationCompletedSignal;

			Wt::WTemplate * pageTemplate;
			Wt::WContainerWidget * votesContainer;

			Wt::WImage * votesRightSideButton;
			Wt::WImage * votesLeftSideButton;
			Wt::WTable * votesTable;
			Wt::WTable * interestedVotesTable;
			RepresentativeInfoWidget * representativeInfo;

			Wt::WText * voteDescriptionArea;
			Wt::WText * voteListDescriptionArea;
			Wt::WText * selectedListDescription;
			Wt::WComboBox * ownedVotesLists;
			Wt::WComboBox * activeMonthList;
			Wt::WComboBox * followedMonthList;
			ShownVotesStateContainer * shownVotesState;
			WGlobals::UserVoteList * userVoteList;
			DataCache & dataContainer;

			std::vector<std::pair<unsigned, unsigned> > currentRepresentativeMatchSum;
			std::map<std::string, std::vector<WDbType::VoteChoise> > mapVoteIdToRepresentativeChoicesVector;

			std::vector<unsigned> ownedVotesIds;
			std::vector<struct UserVoteListId> followListsIds;
			std::string buffer;
			struct CurrentUserListInfo currentUserListInfo;

			void handleVotesLeftSideButtonPressed(void);
			void handleVotesRightSideButtonPressed(void);
			void handleMatchButtonPressed(void);
			void handleVoteSelected(const VotesTableElementWidget * voteInfoWidget);
			void handleMoveToInterestingButtonPresed(const VotesTableElementWidget * voteInfoWidget, bool userChoise);
			void handleRepresentativeCliked(int i);
			void handleShareButtonPressed(void);
			void handleSaveButtonPressed(void);
			void handleDeleteButtonPressed(void);
			void handleOwnedListsListSelected(int index);
			void handleFollowListsListSelected(int index);
			void handleActiveMonthsListSelected(int index);
			void handleVisualizationDone(void);
			void handleRemoveVoteFromVotelistButtonPressed(const Wt::WText * votename);
			void handleVoteFromInterestedTableClicked(const Wt::WText * votename);
			void handleUserChoiseChangedOnVote(std::string voteId);
			void handleShowMoreVotesButtonPressed(void);

			bool initialize(void);
			void createVotesListTables(void);
			void allocateMoreVoteListTableElements(const unsigned numberOfElementsToBeAllocated);
			void getVotesForMonth(const unsigned year, const unsigned & month, bool skipAlreadyShowVotes = false);
			void setPoliticalPartySeatingToBuffer(std::string & buffer, const WGlobals::PoliticalSeasonSeating & seating);
			void setRepresentativeChoisesToBuffer(std::string & buffer, const WGlobals::VoteInformation & vote);
			void setPoliticalPartyNamesToBuffer(std::string & buffer, const WGlobals::PoliticalPartyNames & names);
			void setUserVoteList(const WGlobals::UserVoteList & userVoteList);
			void setPartySeatingInfoToSeatMap(const unsigned & seasonStartingYear);
			void setOwnedListsList(void);
			void setFollowedListsList(void);
			bool setUserVoteList(const std::string & username, const unsigned listId, WGlobals::UserVoteList & userVoteList);
			void createVoteListLink(Wt::WString container, std::string user, unsigned voteId);
			void setActiveMonthsList(const WGlobals::PoliticalSeason & season);
			void doJavascriptInitialization(void);
			void updateWidget(void);
			bool addVoteToUserVoteListWidget(const std::string & voteId, WGlobals::UserVoteList::Vote decision = WGlobals::UserVoteList::NO_OPINION);
			void clearInterestedVotesTable(void);
			void setSetVoteRecordInformation(const std::string & dateOfVote, const int voteNumber);
			Wt::WString createVoteListLink(const std::string & user, unsigned voteId, bool encoded = true);
			bool showOnlyOneVote(const std::string & voteId);
			std::string addRepresentativesChoisesToVoteIdToChoisesMap(const WGlobals::VoteInformation & vote);
			void removeRepresentativesChoisesFromVoteIdToChoisesMap(const std::string & voteId);
			void updateRepresentativeMatch(void);
			void updateRepresentativeMatch(const std::string & voteId);
			void updateRepresentativeMatch(const std::string & voteId, WDbType::UserVoteList::Vote userChoise);
			void setRepresentativeMatchValuesToSeatMap(void);
			void resetcurrentRepresentativeMatchSum(void);
			void setJsLocaleDictionary(void);
			void setSelectedListDescription(const unsigned month, const unsigned year);
			void setSelectedListDescription(const std::string & username, const std::string listName);
			void setVoteStatisticsToBuffer(const WDbType::Dictionary & stats);
	};
}

#endif
