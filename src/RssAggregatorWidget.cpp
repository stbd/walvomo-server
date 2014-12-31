#include "RssAggregatorWidget.hpp"
#include "db/NewsElements.pb.h"
#include "DataCache.hpp"
#include "Logger.hpp"
#include "ContentManager.hpp"

#include <Wt/WTable>
#include <Wt/WText>
#include <Wt/WTemplate>
#include <Wt/WDateTime>

#include <string>
#include <vector>
#include <utility>
#include <cstdio>
#include <algorithm>

using namespace W;

const unsigned DEFAULT_NUMBER_OF_NEWS_ITEMS_ALLOCATED = 20;
const unsigned DEFAULT_NUMBER_OF_NEWS_SOURCE_ITEMS_ALLOCATED = 10;

static const unsigned AGGREGATOR_HTML_LAYOUT_RESOURCE_ID = ContentManager::getInstance()->registerContent("root/static/html/RssAggregatorWidget.html");

namespace W {
class NewsTableElement : public Wt::WContainerWidget
{
public:
	NewsTableElement(Wt::WContainerWidget * parent = 0)
		: Wt::WContainerWidget(parent)
	{
		this->title = new Wt::WText(this);
		this->title->setStyleClass("aggre-element-title");
		this->title->setTextFormat(Wt::XHTMLText);
		new Wt::WBreak(this);
		this->info = new Wt::WText(this);
		this->info->setStyleClass("aggre-element-info");
		this->info->setTextFormat(Wt::XHTMLText);
	}
	void setNewsInformation(const WGlobals::UpdateItem & item, const WGlobals::UpdateSource & source)
	{
		Wt::WDateTime published;
		unsigned ts;
		if (!WGlobals::convertStrToUInt(item.publishedtimestamp(), ts)) {
			W::Log::error() << "Converting published timestamp to uint failed";
			return ;
		}
		published.setTime_t(ts);

		this->title->setText(Wt::WString("<a href=\"{1}\" class=\"aggre-link\">{2}</a>").arg(item.url()).arg(Wt::WString::fromUTF8(item.title())));
		this->info->setText(Wt::WString("{3}: <a class=\"aggre-link\" href=\"{4}\">{5}</a> - {1}: {2}").arg(Wt::WString::tr("aggre.items.published")).
				arg(published.toString("d.M.yyyy H:m")).arg(Wt::WString::tr("aggre.items.source")).arg(source.sourceaddress()).arg(Wt::WString::fromUTF8(source.title())));
	}

private:
	Wt::WText * title;
	Wt::WText * info;
};

class SourceElement : public Wt::WContainerWidget
{
public:
	SourceElement(Wt::WContainerWidget * parent = 0)
		: Wt::WContainerWidget(parent),
		  hasTag(false)
	{
		this->text = new Wt::WText(this);
		this->text->clicked().connect(this, &SourceElement::handleClicked);
		this->tag.reserve(10);
	}

	void setSource(const std::string & title, const std::string & tag)
	{
		this->hasTag = true;
		this->text->setText(Wt::WString::fromUTF8(title));
		this->tag.assign(tag);
	}

	void setSource(const std::string & title)
	{
		this->text->setText(Wt::WString::fromUTF8(title));
	}

	Wt::Signal<const std::string *> & sourceClikked() {return this->sourceClikkedSignal;}

private:
	bool hasTag;
	std::string tag;
	Wt::WText * text;
	Wt::Signal<const std::string *> sourceClikkedSignal;

	void handleClicked()
	{
		if (this->hasTag) {
			W::Log::debug() << "Element with tag " << this->tag << " clicked";
			this->sourceClikkedSignal.emit(&this->tag);
		} else {
			W::Log::debug() << "Element without tag clicked";
		}
	}

};

class NewsSourceTableElement : public Wt::WContainerWidget
{
public:
	NewsSourceTableElement(Wt::WContainerWidget * parent = 0)
		:  Wt::WContainerWidget(parent),
		   hasTag(false),
		   sources(0)
	{
		this->title = new Wt::WText(this);
		this->title->setStyleClass("aggre-sources-high");
		this->title->clicked().connect(this, &NewsSourceTableElement::handleClicked);
		this->tags.reserve(10);
	}

	void setSources(const WGlobals::CollectionOfUpdateSources & collection, RssAggregatorWidget * parrent)
	{
		unsigned i = 0;
		this->title->setText(Wt::WString::fromUTF8(collection.title()));
		this->initSourcesTable(collection.updatesources_size());
		if (collection.has_tags()) {
			this->hasTag = true;
			this->tags.assign(collection.tags());
			//Assumption, done only once
			this->sourceClikked().connect(parrent, &RssAggregatorWidget::handleFilterByTags);
		}
		for (::google::protobuf::RepeatedPtrField<WGlobals::UpdateSource>::const_iterator it = collection.updatesources().begin();
				it != collection.updatesources().end(); ++it) {
			SourceElement * element = (SourceElement *)this->sources->elementAt(i, 0)->widget(0);
			element->sourceClikked().connect(parrent, &RssAggregatorWidget::handleFilterByTags);
			if (it->has_tags()) {
				element->setSource(it->title(), it->tags());
			} else {
				element->setSource(it->title());
			}
			element->show();
			++i;
		}
	}

	Wt::Signal<const std::string *> & sourceClikked() {return this->sourceClikkedSignal;}

private:
	bool hasTag;
	Wt::WText * title;
	Wt::WTable * sources;
	std::string tags;
	Wt::Signal<const std::string *> sourceClikkedSignal;

	void initSourcesTable(const unsigned & numberOfElementsIn) {
		if (this->sources) {delete this->sources;}
		this->sources = new Wt::WTable(this);
		for (unsigned i = 0; i < numberOfElementsIn; ++i) {
			SourceElement * element = new SourceElement();
			element->setStyleClass("aggre-sources-low");
			this->sources->elementAt(i, 0)->addWidget(element);
			element->hide();
		}
	}

	void handleClicked(void)
	{
		if (this->hasTag) {
			W::Log::debug() << "Category with tag " << this->tags << " clicked";
			sourceClikkedSignal.emit(&this->tags);
		} else {
			W::Log::debug() << "Category without tag clicked";
		}
	}
};

struct NewsItem
{
	NewsItem(const WGlobals::UpdateItem & item, const WGlobals::UpdateSource & parrentSource)
		: item(item), parrentSource(parrentSource)
		{}
	const WGlobals::UpdateItem item;
	const WGlobals::UpdateSource parrentSource;
};

typedef std::map<unsigned long, NewsItem>::const_reverse_iterator TimestampValuePair;

class NewsSorter
{
public:
	NewsSorter(const unsigned numberOfItemsShown)
		: prune(false),
		  numberOfItems(numberOfItemsShown)
	{}

	~NewsSorter()
	{
	}

	void clear(void)
	{
		this->shownItems.clear();
		this->lastItemTimestamp = 0;
		this->prune = false;
	}

	void analyseItem(const WGlobals::UpdateItem & item, const WGlobals::UpdateSource & parrentSource)
	{
		this->prune = false;
		this->setItemTimestamp(item);

		if ((this->shownItems.size() == this->numberOfItems) && (this->itemTimestamp < this->lastItemTimestamp)) {
			W::Log::debug() << "Pruning feed \"" << parrentSource.title() << "\" because latest timestamp is greater than newest in feed";
			this->prune = true;
			return ;
		}

		this->shownItems.insert(std::pair<unsigned long, NewsItem>(this->itemTimestamp, NewsItem(item, parrentSource)));
		if (this->shownItems.size() > this->numberOfItems) {
			this->shownItems.erase(this->shownItems.begin());
		}

		this->lastItemTimestamp = this->shownItems.begin()->first;
		if ((this->shownItems.size() == this->numberOfItems) && (this->itemTimestamp <= this->lastItemTimestamp)) {
			W::Log::debug() << "Pruning feed \"" << parrentSource.title() << "\" because queue is full and element has timestamp later than latest in feed";
			this->prune = true;
		}
	}

	inline TimestampValuePair begin() const {return this->shownItems.rbegin();}
	inline TimestampValuePair end() const {return this->shownItems.rend();}
	inline bool shouldRestFromTheSourceBePruned(void) const {return this->prune;}
	WGlobals::UpdateItem itemCache;

private:
	bool prune;
	unsigned numberOfItems;
	unsigned long itemTimestamp;
	unsigned long lastItemTimestamp;

	/*
	 * Sorted by map so that element with greatest ts (that is the newest one) is at end of list
	 */
	std::map<unsigned long, NewsItem> shownItems; //Boost has something called circular buffer

	inline void setItemTimestamp(const WGlobals::UpdateItem & item)
	{
		if (!item.IsInitialized()) {
			throw std::invalid_argument ("Item cache was not set");
		}
		this->itemTimestamp = 0;
		sscanf(item.publishedtimestamp().c_str(), "%lu", &this->itemTimestamp);
		if (this->itemTimestamp == 0) {
			throw std::invalid_argument ("Error converting timestamp to unsigned long");
		}
	}
};
}


RssAggregatorWidget::RssAggregatorWidget(DataCache & dataCache, ContentManager & contentManager)
	: activated(false), newstShownElement(0), dataCache(dataCache)
{
	this->prefix(contentManager);
}

RssAggregatorWidget::~RssAggregatorWidget()
{

}

void RssAggregatorWidget::setActivated()
{
	if (this->activated) {
		W::Log::debug() << "RssAggregatorWidget setActivated when already active";
		return ;
	}
	W::Log::info() << "RssAggregatorWidget setActivated";

	const WGlobals::Collections & collections = this->dataCache.findCollections();	//No need to check for isInit because field is repeated
	setInitialSourcesAndNews(collections);
	this->updateLatestShownElement();
	this->activated = true;
}

void RssAggregatorWidget::handleFilterByTags(const std::string * tags)
{
	W::Log::debug() << "Filtering with tags: " << *tags;

	this->newsSorter->clear();
	this->newstShownElement = 0;
	const WGlobals::Collections & collections = this->dataCache.findCollections();
	for (::google::protobuf::RepeatedPtrField<std::string>::const_iterator c = collections.collections().begin();
			c != collections.collections().end(); ++c) {
		const WGlobals::CollectionOfUpdateSources & collection = this->dataCache.findCollection(*c);
		if (collection.IsInitialized()) {

			W::Log::debug() << "Processing filtering of collection: " << *c;
			if (this->filterCollectionWithTags(collection, *tags)) {
				W::Log::debug() << "Skipping  collection " << *c << " because of filtering tags: " << *tags;
				continue;
			}

			for (::google::protobuf::RepeatedPtrField<WGlobals::UpdateSource>::const_iterator it = collection.updatesources().begin();
					it != collection.updatesources().end(); ++it) {
				W::Log::info()  << "Processing source (filtering): " << it->title();

				if (it->numberofitems() == 0) {
					continue;
				}

				if (this->filterSourceWithTags(*it, *tags)) {
					W::Log::debug() << "Skipping  source " << it->title() << " because of filtering tags: " << *tags;
					continue;
				}

				unsigned long latestTimestamp = 0;
				if (!WGlobals::convertStrTimestampToUInt(it->latestreaditem(), latestTimestamp)) {
					W::Log::warn() << "Error converting " << it->latestreaditem() << " timestamp to uint, ignoring updateSource: " << it->title();
					continue;
				}
				if (this->newstShownElement >= latestTimestamp) {
					W::Log::debug() << "Skipping updateSource because latest item is not newer than latest shown element";
					continue;
				}
				for (unsigned i = it->numberofitems() - 1; i != 0; --i) {
					this->newsSorter->itemCache.Clear();
					this->newsSorter->itemCache = this->dataCache.findUpdateItem(it->datapath(), i);
					if (!this->newsSorter->itemCache.IsInitialized()) {
						W::Log::warn() << "Did not find news item: " << i << " for stream " << it->datapath();
						break;
					}
					this->newsSorter->analyseItem(this->newsSorter->itemCache, *it);
					if (this->newsSorter->shouldRestFromTheSourceBePruned()) {
						break;
					}
				}
			}
		} else {
			W::Log::warn() << "Did not find collection: " << *c;
		}
	}
	this->clearNewsTable();
	this->moveFiltteredElementsToTable();
}

void RssAggregatorWidget::handleShowAllClicked(void)
{
	W::Log::debug() << "Show all cliked";
	static const std::string empty;
	this->handleFilterByTags(&empty);
}

void RssAggregatorWidget::prefix(ContentManager & contentManager)
{
	Wt::WText * sourcesTitle = 0;
	Wt::WText * sourcesInstructions = 0;
	Wt::WText * sourcesShowAllButton = 0;
	Wt::WText * newsTitle = 0;
	Wt::WText * widgetIntro = 0;

	if ((this->pageTemplate = new Wt::WTemplate(contentManager.getContent(AGGREGATOR_HTML_LAYOUT_RESOURCE_ID), this)) == 0) {
		goto memoryAllocationFailed;
	}
	if ((this->newsTable = new Wt::WTable) == 0) {
		goto memoryAllocationFailed;
	}
	this->initNewsTable();
	if ((this->sourcesTable = new Wt::WTable) == 0) {
		goto memoryAllocationFailed;
	}
	this->initNewsSourceTable();

	if ((sourcesTitle = new Wt::WText(Wt::WString::tr("aggre.sources.title"))) == 0) {
		goto memoryAllocationFailed;
	}
	if ((sourcesInstructions = new Wt::WText(Wt::WString::tr("aggre.sources.instructions"))) == 0) {
		goto memoryAllocationFailed;
	}
	if ((sourcesShowAllButton = new Wt::WText(Wt::WString::tr("aggre.sources.show-all"))) == 0) {
		goto memoryAllocationFailed;
	}
	if ((newsTitle = new Wt::WText(Wt::WString::tr("aggre.elements.title"))) == 0) {
		goto memoryAllocationFailed;
	}
	if ((widgetIntro = new Wt::WText(Wt::WString::tr("aggre.sources.intro"))) == 0) {
		goto memoryAllocationFailed;
	}

	this->pageTemplate->bindWidget("RssWidgetInro", widgetIntro);
	this->pageTemplate->bindWidget("elementList", this->newsTable);
	this->pageTemplate->bindWidget("sourceList", this->sourcesTable);
	this->pageTemplate->bindWidget("elementTitle", newsTitle);
	this->pageTemplate->bindWidget("sourceTitle", sourcesTitle);
	this->pageTemplate->bindWidget("sourceInstructions", sourcesInstructions);
	this->pageTemplate->bindWidget("sourceShowAllButton", sourcesShowAllButton);

	newsTitle->setStyleClass("aggre-elements-title");
	sourcesTitle->setStyleClass("aggre-sources-title");
	sourcesInstructions->setStyleClass("aggre-sources-instructions");
	sourcesShowAllButton->setStyleClass("aggre-sources-show-all");
	widgetIntro->setStyleClass("aggre-intro");

	sourcesShowAllButton->clicked().connect(this, &RssAggregatorWidget::handleShowAllClicked);

	if ((this->newsSorter = new NewsSorter(DEFAULT_NUMBER_OF_NEWS_ITEMS_ALLOCATED)) == 0) {
		goto memoryAllocationFailed;
	}

	return ;

	memoryAllocationFailed:
	this->postfix();
	throw std::runtime_error ("Out of memory in RssAggregatorWidget");
}

void RssAggregatorWidget::postfix()
{
	if (this->newsSorter) delete this->newsSorter;
}

void RssAggregatorWidget::initNewsTable()
{
	for (unsigned i = 0; i < DEFAULT_NUMBER_OF_NEWS_ITEMS_ALLOCATED; ++i) {
		NewsTableElement * element = new NewsTableElement;
		this->newsTable->elementAt(i, 0)->addWidget(element);
		element->hide();
	}
}

void RssAggregatorWidget::initNewsSourceTable()
{
	for (unsigned i = 0; i < DEFAULT_NUMBER_OF_NEWS_SOURCE_ITEMS_ALLOCATED; ++i) {
		NewsSourceTableElement * element = new NewsSourceTableElement;
		this->sourcesTable->elementAt(i, 0)->addWidget(element);
		element->hide();
	}
}

void RssAggregatorWidget::setInitialSourcesAndNews(const WGlobals::Collections & collections)
{
	for (::google::protobuf::RepeatedPtrField<std::string>::const_iterator it = collections.collections().begin();
			it != collections.collections().end(); ++it) {
		const WGlobals::CollectionOfUpdateSources & collection = this->dataCache.findCollection(*it);
		if (collection.IsInitialized()) {
			W::Log::debug() << "Processing collection: " << *it;
			this->setSourceCollection(collection);
			this->updateNewsTable(collection);
		} else {
			W::Log::warn() << "Did not find collection: " << *it;
		}
	}
	this->moveFiltteredElementsToTable();
}

void RssAggregatorWidget::setSourceCollection(const WGlobals::CollectionOfUpdateSources & collection)
{
	for (unsigned i = 0; i < (unsigned)this->sourcesTable->rowCount(); ++i) {
		NewsSourceTableElement * element = (NewsSourceTableElement *)this->sourcesTable->elementAt(i, 0)->widget(0);
		if (!element->isHidden()) {
			continue;
		}
		element->setSources(collection, this);
		element->show();
		break;
	}
}

void RssAggregatorWidget::updateNewsTable(const WGlobals::CollectionOfUpdateSources & collection)
{
	for (::google::protobuf::RepeatedPtrField<WGlobals::UpdateSource>::const_iterator it = collection.updatesources().begin();
			it != collection.updatesources().end(); ++it) {
		W::Log::debug()  << "Processing source: " << it->title();

		if (it->numberofitems() == 0) {
			continue;
		}
		unsigned long latestTimestamp = 0;
		if (!WGlobals::convertStrTimestampToUInt(it->latestreaditem(), latestTimestamp)) {
			W::Log::warn() << "Error converting " << it->latestreaditem() << " timestamp to uint, ignoring updateSource: " << it->title();
			continue;
		}
		if (this->newstShownElement >= latestTimestamp) {
			W::Log::debug() << "Skipping updateSource because latest item is not newer than latest shown element";
			continue;
		}
		for (unsigned i = it->numberofitems() - 1; i != 0; --i) {
			this->newsSorter->itemCache.Clear();
			this->newsSorter->itemCache = this->dataCache.findUpdateItem(it->datapath(), i);
			if (!this->newsSorter->itemCache.IsInitialized()) {
				W::Log::warn() << "Did not find news item: " << i << " for stream " << it->datapath();
				break;
			}
			this->newsSorter->analyseItem(this->newsSorter->itemCache, *it);
			if (this->newsSorter->shouldRestFromTheSourceBePruned()) {
				break;
			}
		}
	}
}

bool RssAggregatorWidget::addNewsItemToTable(const WGlobals::UpdateItem & item, const WGlobals::UpdateSource & source)
{
	for (unsigned i = 0; i < (unsigned)this->newsTable->rowCount(); ++i) {
		NewsTableElement * element = (NewsTableElement *)this->newsTable->elementAt(i, 0)->widget(0);
		if (!element->isHidden()) {
			continue ;
		}
		element->setNewsInformation(item, source);
		element->show();
		return true;
	}
	return false;
}

void RssAggregatorWidget::updateLatestShownElement(void)
{
	if (this->newsSorter->begin() != this->newsSorter->end()) {
		this->newstShownElement = this->newsSorter->begin()->first;
	}
}

void RssAggregatorWidget::clearNewsTable(void)
{
	for (unsigned i = 0; i < (unsigned)this->newsTable->rowCount(); ++i) {
		NewsTableElement * element = (NewsTableElement *)this->newsTable->elementAt(i, 0)->widget(0);
		if (element->isHidden()) {
			continue ;
		}
		element->hide();
	}
}

void RssAggregatorWidget::moveFiltteredElementsToTable(void)
{
	for (TimestampValuePair it = this->newsSorter->begin();
			it != this->newsSorter->end(); ++it) {
		if (!it->second.item.IsInitialized()) {
			break;
		}
		if (!this->addNewsItemToTable(it->second.item, it->second.parrentSource)) {
			break;
		}
	}
}

bool RssAggregatorWidget::filterSourceWithTags(const WGlobals::UpdateSource & source, const std::string & tags)
{
	if (!source.has_tags() || tags.empty()) {
		return false;
	}
#ifdef DEBUG
	W::Log::debug() << "filterSourceWithTags: " << source.tags() << " " << tags;
#endif
	return areTagsFoundInStr(source.tags(), tags, 's');
}

bool RssAggregatorWidget::filterCollectionWithTags(const WGlobals::CollectionOfUpdateSources & collection, const std::string & tags)
{
	if (!collection.has_tags() || tags.empty()) {
		return false;
	}
#ifdef DEBUG
	W::Log::debug() << "filterSourceWithTags: " << collection.tags() << " " << tags;
#endif
	return areTagsFoundInStr(collection.tags(), tags, 'c');
}

bool RssAggregatorWidget::areTagsFoundInStr(const std::string & str, const std::string & tags, const char prefix)
{
	std::string::const_iterator it = tags.begin();
	while (1) {
		std::string::const_iterator start = std::find(it, tags.end(), prefix);
		if (start == tags.end()) {
			break;
		}
		++start;
		if (*start != ':') {
			it = start;
			continue;
		}
		--start;
		std::string::const_iterator end = std::find(start, tags.end(), ';');
		if (end == tags.end()) {
			break;
		}
		++end;
#ifdef DEBUG
		W::Log::debug() << "Found tag " << std::string(start, end) << " in " << str << " = " << !(str.find(std::string(start, end)) == std::string::npos);
#endif

		if (str.find(std::string(start, end)) == std::string::npos) {
			return true;
		}
		it = end;
	}
	return false;
}
