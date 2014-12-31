#include "ContentManager.hpp"
#include "Logger.hpp"

#include <stdexcept>
#include <fstream>

using namespace W;

ContentManager * ContentManager::instance = 0;

ContentManager::ContentManager(void)
	: nextContentId(1), totalContentSizeBytes(0)
{
}

ContentManager::~ContentManager(void)
{
	this->contents.clear();
	W::Log::info() << "ContentManager released";
}

ContentManager * ContentManager::getInstance()
{
	if (instance) {
		return instance;
	} else {
		W::Log::info() << "Creating new content manager";
		if ((instance = new ContentManager()) == 0) {
			throw std::runtime_error ("ContentManager: Failed to create ContentManager");
		}
		return instance;
	}
}

const char * ContentManager::getContent(const unsigned id)
{
	std::map<unsigned, std::vector<char> >::const_iterator it = this->contents.find(id);
	if (it != this->contents.end()) {
		return &(*it->second.begin());
	} else {
		throw std::runtime_error ("ContentManager: Key does not exist");
	}
}

unsigned ContentManager::registerContent(const char * pathToContentFile)
{
	unsigned contentId = this->nextContentId;
	this->loadContent(pathToContentFile, contentId);
	++this->nextContentId;

	if (contentId > this->nextContentId) {
		throw std::runtime_error ("nextContentId overflow");
	}
	return contentId;
}

void ContentManager::loadContent(const char * filename, const unsigned id)
{
#ifdef DEBUG
	W::Log::info() << "Loading content from " << filename << " with id " << id;
#endif

	if (this->contents.count(id) != 0) {
		throw std::runtime_error ("ContentManager: Key already exists");
	}

	std::fstream f(filename);
	if (!f.is_open() || !f.good()) {
		throw std::runtime_error ("ContentManager: Unable open content file");
	}

	unsigned size = 0;
	std::vector<char> data;

	f.seekg(0, std::ios_base::end);
	size = f.tellg();
	f.seekg(0, std::ios_base::beg);

	if (size == 0) {
		throw std::runtime_error ("ContentManager: content file size zero");
	}

	data.resize(size + 1);
	data[size] = '\0';
	f.read(&data[0], size);
	if ((unsigned)f.gcount() != size) {
		throw std::runtime_error ("ContentManager: Read bytes from content file");
	}

	this->contents.insert( std::pair<unsigned, std::vector<char> >(id, data) );
	this->totalContentSizeBytes += size;
}
