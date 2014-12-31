#ifndef RSS_AGGREGATOR_WIDGET_HPP
#define RSS_AGGREGATOR_WIDGET_HPP

#include "Globals.hpp"

#include <Wt/WContainerWidget>

namespace {
	class WTable;
	class WTemplate;
	class WText;
}

namespace W {

class ContentManager;
class DataCache;
class NewsSorter;

class RssAggregatorWidget : public Wt::WContainerWidget
{
	public:
		RssAggregatorWidget(DataCache & dataCache, ContentManager & contentManager);
		~RssAggregatorWidget();
		void setActivated();

		void handleFilterByTags(const std::string * tags);
	private:
		void handleShowAllClicked(void);

		void prefix(ContentManager & contentManager);
		void postfix();
		void initNewsTable();
		void initNewsSourceTable();
		void setInitialSourcesAndNews(const WGlobals::Collections & collections);
		void setSourceCollection(const WGlobals::CollectionOfUpdateSources & collection);
		void updateNewsTable(const WGlobals::CollectionOfUpdateSources & collection);
		bool addNewsItemToTable(const WGlobals::UpdateItem & item, const WGlobals::UpdateSource & source);
		void updateLatestShownElement(void);
		void clearNewsTable(void);
		void moveFiltteredElementsToTable(void);
		bool filterSourceWithTags(const WGlobals::UpdateSource & source, const std::string & tags);
		bool filterCollectionWithTags(const WGlobals::CollectionOfUpdateSources & collection, const std::string & tags);
		bool areTagsFoundInStr(const std::string & tags, const std::string & tag, const char prefix);

		bool activated;
		unsigned long newstShownElement;
		Wt::WTemplate * pageTemplate;
		Wt::WTable * newsTable;
		Wt::WTable * sourcesTable;
		NewsSorter * newsSorter;
		DataCache & dataCache;
};


}

#endif
