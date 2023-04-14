/**
 * @file CoreManager_test.cxx
 *
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE CoreManager_test // NOLINT

#include "boost/test/unit_test.hpp"

#include "coremanager/CoreManager.hpp"

using namespace dunedaq::coremanager;

BOOST_AUTO_TEST_SUITE(CoreManager_test)

BOOST_AUTO_TEST_CASE(allocate01) {
  //BOOST_TEST_MESSAGE("");
  CoreManager::get()->configure("0,1");

  bool caughtException;
  for (unsigned int i=0; i<3; ++i) {
    caughtException=false;
    try {
      CoreManager::get()->allocate("CoreManager_test", 1);
    }
    catch (ers::Issue& issue) {
      caughtException=true;
    }
    if (i<2) {
      BOOST_CHECK(caughtException==false);
      BOOST_CHECK(CoreManager::get()->allocated()==i+1);
      BOOST_CHECK(CoreManager::get()->available()==2-(i+1));
    }
    else {
      BOOST_CHECK(caughtException==true);
      BOOST_CHECK(CoreManager::get()->allocated()==i);
      BOOST_CHECK(CoreManager::get()->available()==0);
    }
  }
  CoreManager::get()->reset();
}

BOOST_AUTO_TEST_CASE(configure) {
  CoreManager::get()->dump();

  BOOST_TEST_MESSAGE("Reconfiguring CoreManager");
  CoreManager::get()->configure("0-3");
  CoreManager::get()->dump();

  CoreManager::get()->reset();
  BOOST_TEST_MESSAGE("Reconfiguring CoreManager");
  CoreManager::get()->configure("0,1..3");
  CoreManager::get()->dump();

  CoreManager::get()->reset();
}

BOOST_AUTO_TEST_CASE(release) {
  BOOST_TEST_MESSAGE("Testing release method");
  std::string name = "releaseTest";
  CoreManager::get()->configure("0,1..3");
  CoreManager::get()->allocate(name, 1);
  BOOST_CHECK(CoreManager::get()->allocated()==1);

  CoreManager::get()->release(name);
  BOOST_CHECK(CoreManager::get()->allocated()==0);
}
BOOST_AUTO_TEST_SUITE_END()
