/*
 * XenStoreStub.hpp
 *
 *  Created on: Feb 28, 2017
 *      Author: al1
 */

#ifndef TEST_MOCKS_XENSTOREMOCK_HPP_
#define TEST_MOCKS_XENSTOREMOCK_HPP_

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include "Pipe.hpp"

class XenStoreMock
{
public:

	~XenStoreMock();

	static XenStoreMock* getInstance();

	int getFd() const { return mPipe.getFd(); }
	void setDomainPath(unsigned int domId, const std::string& path);
	const char* getDomainPath(unsigned int domId);
	void writeValue(const std::string& path, const std::string& value);
	const char* readValue(const std::string& path);
	bool deleteEntry(const std::string& path);
	std::vector<std::string> readDirectory(const std::string& path);
	bool watch(const std::string& path);
	bool unwatch(const std::string& path);
	bool getChangedEntry(std::string& path);

private:

	XenStoreMock() {}

	static XenStoreMock* sInstance;

	Pipe mPipe;

	std::unordered_map<unsigned int, std::string> mDomPathes;
	std::unordered_map<std::string, std::string> mEntries;
	std::list<std::string> mWatches;
	std::list<std::string> mChangedEntries;
};

#endif /* TEST_MOCKS_XENSTOREMOCK_HPP_ */
