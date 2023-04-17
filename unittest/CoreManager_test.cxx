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

#include <sstream>

using namespace dunedaq::coremanager;

BOOST_AUTO_TEST_SUITE(CoreManager_test)

BOOST_AUTO_TEST_CASE(allocate) {
  //BOOST_TEST_MESSAGE("");
  CoreManager::get()->configure("0,1,3");

  bool caughtException;
  for (unsigned int i=0; i<3; ++i) {
    caughtException=false;
    try {
      CoreManager::get()->allocate("CoreManager_test", 1);
    }
    catch (ers::Issue& issue) {
      caughtException=true;
    }
    BOOST_CHECK(CoreManager::get()->allocated()==i+1);
    BOOST_CHECK(CoreManager::get()->available()==3-(i+1));
    if (i<2) {
      BOOST_CHECK(caughtException==false);
    }
    else {
      BOOST_CHECK(caughtException==true);
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
  CoreManager::get()->configure("0,1..3");

  std::vector<std::thread> test;
  bool active[4];
  std::vector<std::string > name;
  for (unsigned int i=0; i<3; ++i) {
    std::ostringstream num;
    num.str("");
    num << i;
    name.emplace_back("releaseTest" + num.str());
    CoreManager::get()->allocate(name[i], 1);
  }
  for (unsigned int i=0; i<3; ++i) {
    active[i]=false;
    bool* flag=&active[i];
    test.emplace_back(std::thread([flag,name,i](){
      CoreManager::get()->setAffinity(name[i]);
      std::cout << name[i] << ": "
                << CoreManager::get()->affinityString() << std::endl;
      *flag=true;
      while (*flag) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
      CoreManager::get()->release(name[i]);
      std::cout << name[i] << ": "
                << CoreManager::get()->affinityString() << std::endl;
    }));
  }
  std::cout << "Main: " << CoreManager::get()->affinityString() << std::endl;

  bool ready=false;
  while (!ready) {
    ready=true;
    for (unsigned int i=0; i<3; ++i) {
      ready &= active[i];
    }
    if (!ready) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
  BOOST_CHECK(CoreManager::get()->allocated()==3);
  std::cout << "Main: " << CoreManager::get()->affinityString() << std::endl;

  for (unsigned int i=0; i<3; ++i) {
    active[i]=false;
    test[i].join();
  }

  BOOST_CHECK(CoreManager::get()->allocated()==0);
  std::cout << "Main: " << CoreManager::get()->affinityString() << std::endl;
}
BOOST_AUTO_TEST_SUITE_END()
