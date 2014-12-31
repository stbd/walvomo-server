#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#ifndef NO_WT

#include <Wt/WApplication>

namespace W
{
	class Log
	{
		public:
			static Wt::WLogEntry info() {return Wt::log("info");}
			static Wt::WLogEntry warn() {return Wt::log("warn");}
			static Wt::WLogEntry error() {return Wt::log("error");}
			static Wt::WLogEntry debug() {return Wt::log("debug");}
	};
}

#else

#include <iostream>

namespace W
{
  class Log
  {
  public:
    static std::ostream & info() {return std::cout;}
    static std::ostream & warn() {return std::cout;}
    static std::ostream & error() {return std::cout;}
    static std::ostream & debug() {return std::cout;}
  };
}
#endif

#endif
