#include "MatchAlgorithm.hpp"
#include "Logger.hpp"

#include <stdexcept>

using namespace W;

unsigned W::match(const std::map<unsigned, struct RepresentativeVotelistInfo> & candidatesVotes, const std::vector<bool> & userVotes, std::vector<float> & matchPercentage)
{
	if (matchPercentage.capacity() != candidatesVotes.size()) {
		matchPercentage.resize(candidatesVotes.size(), 0.0f);
	}

	unsigned bestMatchIndex = 0;
	float bestMatch = 0.0f;

	for (std::map<unsigned, struct RepresentativeVotelistInfo>::const_iterator it = candidatesVotes.begin(); it != candidatesVotes.end(); ++it) {

		if (it->second.votes.size() != userVotes.size()) {
			W::Log::error() << "Size mismatch for candidatesVotes: " << it->second.votes.size() << " != " << userVotes.size();
			throw std::invalid_argument ("Size mismatch for candidatesVotes");
		}

		W::Log::debug() << "Handling representative with id " << it->first << " in match algorithm";

		float match = 0.0f;
		std::vector<bool>::const_iterator candIt = it->second.votes.begin();
		std::vector<bool>::const_iterator candValidIt = it->second.votesValidity.begin();
		std::vector<bool>::const_iterator userIt = userVotes.begin();

		while ((candIt != it->second.votes.end()) && (candValidIt != it->second.votesValidity.end()) && (userIt != userVotes.end())) {
			if (*candValidIt) {
				match += *candIt == *userIt;
			}
			++candIt;++candValidIt;++userIt;
		}
		match /= (float)userVotes.size();
		match *= 100.0f;					//To percentage
		unsigned index = std::distance(std::map<unsigned, struct RepresentativeVotelistInfo>::const_iterator(candidatesVotes.begin()), it);
		matchPercentage[index] = match;
		if (match > bestMatch) {
			bestMatch = match;
			bestMatchIndex = index;
		}
	}
	return bestMatchIndex;
}

