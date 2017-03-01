/*
 * test.hpp
 *
 *  Created on: Feb 28, 2017
 *      Author: al1
 */

#ifndef TEST_TEST_HPP_
#define TEST_TEST_HPP_

#include <BackendBase.hpp>

class TestBackend : public XenBackend::BackendBase
{
public:

	TestBackend() : XenBackend::BackendBase("TestBackend", "test", 12, 3) {}

private:

	void onNewFrontend(domid_t domId, uint16_t devId) override {}

};

#endif /* TEST_TEST_HPP_ */
