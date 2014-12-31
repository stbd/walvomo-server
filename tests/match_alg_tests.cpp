#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MatchAlgTest
#include <boost/test/unit_test.hpp>
#include "../src/MatchAlgorithm.hpp"
#include <vector>
#include <map>
#include <iostream>

using namespace Oc;
static bool print = false;

void logMatches(unsigned bestMatch, std::vector<float> matches)
{
  if (!print) {
    return ;
  }
  std::cout << "\n\n\nBest match (index): " << bestMatch << "\n";

  unsigned i = 1;
  for (std::vector<float>::const_iterator it = matches.begin(); it != matches.end(); ++it) {
    std::cout << "Rep " << i << " (index: " << i-1 << ") match: " << *it << "\n";
    ++i;
  }
}

/*
 * Two representatives with 2 votes both of which they were absent, so the votes are invalid
 */

BOOST_AUTO_TEST_CASE( SanityCheck1 )
{
  //Rep1:
  struct RepresentativeVotelistInfo rep1(2);
  rep1.votes[0] = true;
  rep1.votes[1] = false;
  rep1.votesValidity[0] = false;
  rep1.votesValidity[1] = false;

  //Rep2:
  struct RepresentativeVotelistInfo rep2(2);
  rep2.votes[0] = true;
  rep2.votes[1] = false;
  rep2.votesValidity[0] = false;
  rep2.votesValidity[1] = false;

  //User
  std::vector<bool> userVotes;
  userVotes.push_back(true);
  userVotes.push_back(false);

  //Combine
  std::map<unsigned, struct RepresentativeVotelistInfo> votes;
  votes.insert(std::pair<unsigned, RepresentativeVotelistInfo>(1, rep1));
  votes.insert(std::pair<unsigned, RepresentativeVotelistInfo>(2, rep2));
  std::vector<float> matches;
  unsigned bestMatch = Oc::match(votes, userVotes, matches);

  logMatches(bestMatch, matches);
  BOOST_CHECK(bestMatch == 0);
  BOOST_CHECK(matches.size() == 2);
  BOOST_CHECK((int)matches[0] == 0);
  BOOST_CHECK((int)matches[1] == 0);
}

/*
 * Two representatives with two valid botes both, rep 2 with same opinions as user, rep 1 opposite.
 */ 
BOOST_AUTO_TEST_CASE( SanityCheck2 )
{
  //Rep1:
  struct RepresentativeVotelistInfo rep1(2);
  rep1.votes[0] = false;
  rep1.votes[1] = true;
  rep1.votesValidity[0] = true;
  rep1.votesValidity[1] = true;

  //Rep2:
  struct RepresentativeVotelistInfo rep2(2);
  rep2.votes[0] = true;
  rep2.votes[1] = false;
  rep2.votesValidity[0] = true;
  rep2.votesValidity[1] = true;

  //User
  std::vector<bool> userVotes;
  userVotes.push_back(true);
  userVotes.push_back(false);

  //Combine
  std::map<unsigned, struct RepresentativeVotelistInfo> votes;
  votes.insert(std::pair<unsigned, RepresentativeVotelistInfo>(1, rep1));
  votes.insert(std::pair<unsigned, RepresentativeVotelistInfo>(2, rep2));
  std::vector<float> matches;
  unsigned bestMatch = Oc::match(votes, userVotes, matches);

  logMatches(bestMatch, matches);
  BOOST_CHECK(bestMatch == 1);
  BOOST_CHECK((int)matches[bestMatch] == 100);
  BOOST_CHECK(matches.size() == 2);
  BOOST_CHECK((int)matches[0] == 0);
  BOOST_CHECK((int)matches[1] == 100);
}

/*
 * Three representatives with two valid votes all, rep 2 with same opinions as user, rep 1 opposite, rep 3 in the midlle.
 */ 

BOOST_AUTO_TEST_CASE( SanityCheck4 )
{
  //Rep1:
  struct RepresentativeVotelistInfo rep1(2);
  rep1.votes[0] = false;
  rep1.votes[1] = true;
  rep1.votesValidity[0] = true;
  rep1.votesValidity[1] = true;

  //Rep2:
  struct RepresentativeVotelistInfo rep2(2);
  rep2.votes[0] = true;
  rep2.votes[1] = false;
  rep2.votesValidity[0] = true;
  rep2.votesValidity[1] = true;

  //Rep3
  struct RepresentativeVotelistInfo rep3(2);
  rep3.votes[0] = true;
  rep3.votes[1] = true;
  rep3.votesValidity[0] = true;
  rep3.votesValidity[1] = true;

  //User
  std::vector<bool> userVotes;
  userVotes.push_back(true);
  userVotes.push_back(false);

  std::map<unsigned, struct RepresentativeVotelistInfo> votes;
  votes.insert(std::pair<unsigned, RepresentativeVotelistInfo>(1, rep1));
  votes.insert(std::pair<unsigned, RepresentativeVotelistInfo>(2, rep2));
  votes.insert(std::pair<unsigned, RepresentativeVotelistInfo>(3, rep3));
  std::vector<float> matches;
  unsigned bestMatch = Oc::match(votes, userVotes, matches);

  logMatches(bestMatch, matches);
  BOOST_CHECK(bestMatch == 1);
  BOOST_CHECK((int)matches[bestMatch] == 100);
  BOOST_CHECK(matches.size() == 3);
  BOOST_CHECK((int)matches[0] == 0);
  BOOST_CHECK((int)matches[1] == 100);
  BOOST_CHECK((int)matches[2] == 50);
}


BOOST_AUTO_TEST_CASE( RepWithInvalidNumberOfVotes  )
{
  //Rep1:
  struct RepresentativeVotelistInfo rep1(1);
  rep1.votes[0] = false;
  rep1.votesValidity[0] = true;

  //User
  std::vector<bool> userVotes;
  userVotes.push_back(true);
  userVotes.push_back(false);

  std::map<unsigned, struct RepresentativeVotelistInfo> votes;
  votes.insert(std::pair<unsigned, RepresentativeVotelistInfo>(1, rep1));
  std::vector<float> matches;
  bool trown = false;
  
  try {
    Oc::match(votes, userVotes, matches);
  } catch (std::exception &) {
    trown = true;
  }

  BOOST_CHECK(trown);
}

