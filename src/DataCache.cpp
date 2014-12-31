#include "DataCache.hpp"
#include "Logger.hpp"

#include "Logger.hpp"
#include "DatabaseSession.hpp"
#include "db/VoteInfo.pb.h"
#include "db/RepresentativeInfo.pb.h"
#include "db/PoliticalSeason.pb.h"

#include <stdexcept>

using namespace W;

DataCache::DataCache(DatabaseSession & session)
	: startingYearOfSeasonForIdMap(0), numberOfAdditionalSeats(0), session(session)
{

}

const WGlobals::ListOfVotesForMonth & DataCache::findVotesForMonth(const unsigned year, const unsigned month)
{
	return this->session.findVotesForMonth(year, month);
}

const WGlobals::VoteInformation & DataCache::findVote(const std::string & id)
{
	return this->session.findVote(id.c_str(), id.length());
}

const WGlobals::VoteStatistics & DataCache::findVoteStatistics(const std::string & id)
{
	return this->session.findVoteStatistics(id.c_str(), id.length());
}

const WGlobals::PoliticalSeasonSeating & DataCache::findSeatingForSeason(const unsigned startingYear)
{
	return this->session.findSeatingForSeason(startingYear);
}

const WGlobals::RepresentativeInfo & DataCache::findRepresentativeInfo(const unsigned id)
{
	return this->session.findRepresentativeInfo(id);
}

const WGlobals::PoliticalSeasonsList & DataCache::findPoliticalSeasonsList(void)
{
	return this->session.findPoliticalSeasonsList();
}

const WGlobals::PoliticalSeason & DataCache::findPoliticalSeasonInfo(const unsigned startingYear)
{
	const WGlobals::PoliticalSeason & season = this->session.findPoliticalSeason(startingYear);
	if (this->startingYearOfSeasonForIdMap != startingYear) {
		this->startingYearOfSeasonForIdMap = startingYear;
		const WGlobals::PoliticalSeasonSeating & seating =  this->findSeatingForSeason(startingYear);
		if (!seating.IsInitialized()) {
			W::Log::error() << "Seating for " << startingYear << " was not found in findVotesForMonth";
		} else {
			this->setIdToSeatMap(seating);
		}
	}
	return season;
}

const WGlobals::PoliticalPartyNames & DataCache::findPoliticalPartyNameInfo(void)
{
	return this->session.findPoliticalPartyNameInfo();
}

const WGlobals::VoteRecord & DataCache::findVoteRecord(const std::string & date)
{
	return this->session.findVoteRecord(date.c_str(), date.length());
}

const WGlobals::Collections & DataCache::findCollections(void)
{
	return this->session.findCollections();
}

const WGlobals::CollectionOfUpdateSources & DataCache::findCollection(const std::string & name)
{
	return this->session.findCollectionOfUpdateSources(name.c_str(), name.length());
}

const WGlobals::UpdateSource & DataCache::findUpdateSOurce(const std::string & name)
{
	return this->session.findUpdateSource(name.c_str(), name.length());
}

const WGlobals::UpdateItem & DataCache::findUpdateItem(const std::string & baseName, const unsigned number)
{
	return this->session.findUpdateItem(baseName.c_str(), baseName.length(), number);
}

unsigned DataCache::convertIdToSeat(unsigned id)
{
	std::map<unsigned, unsigned>::const_iterator it = this->idToSeatMap.find(id);
	if (it == this->idToSeatMap.end()) {
		W::Log::error() << "Did not find politician with id " << id;
		throw std::invalid_argument ("Invalid politician id");
	}
	return it->second;
}

unsigned DataCache::convertSeatToId(unsigned seat)
{
	bool found = false;
	unsigned id = 0;
	for (std::map<unsigned, unsigned>::const_iterator it = this->idToSeatMap.begin(); it != this->idToSeatMap.end(); ++it) {
		if (it->second == seat) {
			found = true;
			id = it->first;
			break;
		}
	}
	if (!found) {
		throw std::invalid_argument ("Did not find id with seat");
	}
	return id;
}

void DataCache::setIdToSeatMap(const WGlobals::PoliticalSeasonSeating & seating)
{
	this->idToSeatMap.clear();
	for (google::protobuf::RepeatedPtrField<WDbType::RepresentativeSeatingInfo>::const_iterator it = seating.representativeseat().begin(); it != seating.representativeseat().end(); ++it) {
		this->idToSeatMap.insert(std::pair<unsigned, unsigned>(it->representativekey(), it->seat()));
	}
	for (google::protobuf::RepeatedPtrField<WDbType::RepresentativeSeatChangeInfo>::const_iterator it = seating.seatchange().begin(); it != seating.seatchange().end(); ++it) {
		this->idToSeatMap.insert(std::pair<unsigned, unsigned>(it->representativeseat().representativekey(), it->representativeseat().seat()));
	}
	this->numberOfAdditionalSeats = seating.seatchange_size();
}

