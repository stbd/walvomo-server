#ifndef DATA_CONTAINER_HPP_
#define DATA_CONTAINER_HPP_

#include "Globals.hpp"

#include <string>
#include <map>
#include <vector>
#include <ctime>

namespace W
{
	class DatabaseSession;

	struct SeatChangeInfo
	{
		SeatChangeInfo(const unsigned id, const unsigned seat, const time_t fromTimestamp)
			: id(id), seat(seat), fromTimestamp(fromTimestamp) {}
		unsigned id;
		unsigned seat;
		std::time_t fromTimestamp;
	};

	class DataCache
	{
		public:
			DataCache(DatabaseSession & session);
			const WGlobals::ListOfVotesForMonth & findVotesForMonth(const unsigned year, const unsigned month);
			const WGlobals::VoteInformation & findVote(const std::string & id);
			const WGlobals::VoteStatistics & findVoteStatistics(const std::string & id);
			const WGlobals::PoliticalSeasonSeating & findSeatingForSeason(const unsigned startingYear);
			const WGlobals::RepresentativeInfo & findRepresentativeInfo(const unsigned id);
			const WGlobals::PoliticalSeasonsList & findPoliticalSeasonsList(void);
			const WGlobals::PoliticalSeason & findPoliticalSeasonInfo(const unsigned startingYear);
			const WGlobals::PoliticalPartyNames & findPoliticalPartyNameInfo(void);
			const WGlobals::VoteRecord & findVoteRecord(const std::string & date);
			const WGlobals::Collections & findCollections(void);
			const WGlobals::CollectionOfUpdateSources & findCollection(const std::string & name);
			const WGlobals::UpdateSource & findUpdateSOurce(const std::string & name);
			const WGlobals::UpdateItem & findUpdateItem(const std::string & baseName, const unsigned number);

			inline unsigned numberOfAdditionalSeatsInSeason(void) {return this->numberOfAdditionalSeats;}
			unsigned convertIdToSeat(unsigned id);
			unsigned convertSeatToId(unsigned seat);
		private:
			unsigned startingYearOfSeasonForIdMap;
			unsigned numberOfAdditionalSeats;
			std::map<unsigned, unsigned> idToSeatMap;
			void setIdToSeatMap(const WGlobals::PoliticalSeasonSeating & seating);

			DatabaseSession & session;
	};

}

#endif
