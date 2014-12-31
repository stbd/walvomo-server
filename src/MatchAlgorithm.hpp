#ifndef MATCH_ALGORITHM_HPP_
#define MATCH_ALGORITHM_HPP_

#include <vector>
#include <map>
#include <cstddef>

namespace W
{
	struct RepresentativeVotelistInfo
	{
		RepresentativeVotelistInfo(const size_t numberOfVotes)
			: votes(numberOfVotes, false), votesValidity(numberOfVotes, false) {}

		std::vector<bool> votes;
		std::vector<bool> votesValidity;
	};
	unsigned match(const std::map<unsigned, struct RepresentativeVotelistInfo> & candidatesVotes, const std::vector<bool> & userVotes, std::vector<float> & matchPercentage);
}

#endif
