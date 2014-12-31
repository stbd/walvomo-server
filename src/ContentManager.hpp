#ifndef CONTENT_MANAGER_HPP_
#define CONTENT_MANAGER_HPP_

#include <map>
#include <vector>

//Singleton

namespace W
{
	//Temp solution, <not elegant>
	static const unsigned MAIN_LAYOUT_CONTENT_ID = 1;
	static const unsigned FRONTPAGE_CONTENT_ID = 2;
	static const unsigned VOTE_MAP_ID = 3;

	//</elegant>

	static const char * const PATH_TO_CONTENT_FILES = "static/";

	class ContentManager
	{
		public:
			static ContentManager * getInstance();
			const char * getContent(const unsigned id);
			unsigned registerContent(const char * pathToContentFile);
			inline unsigned getContentByteSize(void) {return this->totalContentSizeBytes;}
			~ContentManager(void);

		private:
			static ContentManager * instance;
			unsigned nextContentId;
			unsigned totalContentSizeBytes;
			std::map<unsigned, std::vector<char> > contents;

			ContentManager(void);
			ContentManager(const ContentManager & ) {}; //Not available
			void loadContent(const char * name, const unsigned id);
	};
}

#endif
