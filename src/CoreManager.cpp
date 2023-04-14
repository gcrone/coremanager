/**
 * @file CoreManager.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "coremanager/CoreManager.hpp"

#include <memory>
#include <regex>
#include <iostream>

#include <pthread.h>

namespace dunedaq::coremanager {
  std::shared_ptr<CoreManager> CoreManager::s_instance = nullptr;

void CoreManager::configure(const std::string& corelist) {
  static const std::regex re(
    R"(((\d+)-(\d+))|((\d+)\.\.(\d+))|\d+)");
  std::smatch sm;
  for (std::string s(corelist); std::regex_search(s, sm, re); s = sm.suffix()) {
    if (sm[2].length()) {
      for (int i = std::stoi(sm[2]); i <= std::stoi(sm[3]); ++i) {
        m_availableCores.push_back(i);
      }
    }
    else if (sm[5].length()) {
      for (int i = std::stoi(sm[5]); i <= std::stoi(sm[6]); ++i) {
        m_availableCores.push_back(i);
      }
    }
    else {
      m_availableCores.push_back(std::stoi(sm[0]));
    }
  }
  m_configured = true;
}

void CoreManager::allocate(const std::string& name, unsigned int ncores) {
  if (m_availableCores.size() >= ncores) {
    if (m_allocations.find(name) == m_allocations.end()) {
      m_allocations[name] = std::vector<int>();
    }
    cpu_set_t pmask;
    CPU_ZERO(&pmask);
    for (unsigned int i=0; i<ncores; ++i) {
      int core = m_availableCores.back();
      m_availableCores.pop_back();
      CPU_SET(core, &pmask);
      m_allocations[name].push_back(core);
      m_nallocations++;
    }
    auto status=sched_setaffinity(0, sizeof(pmask), &pmask);
    if (status != 0) {
      // throw an ers Issue
      throw (AffinitySettingFailed(ERS_HERE));
    }
  }
  else {
    // throw an ers issue
    throw (AllocationFailed(ERS_HERE));
  }
}

void CoreManager::release(const std::string& name) {
  cpu_set_t pmask;
  auto status = sched_getaffinity(0, sizeof(pmask), &pmask);
  if (status != 0) {
    throw (AffinityGettingFailed(ERS_HERE));
  }
  if (CPU_COUNT(&pmask) == 0) {
    throw (AffinityNotSet(ERS_HERE));
  }
  for (int mycore = 0; mycore < CPU_SETSIZE; mycore++) {
    if (CPU_ISSET(mycore, &pmask)) {
      for (auto iter=m_allocations[name].begin(); iter!=m_allocations[name].end(); iter++) {
        if (*iter == mycore) {
          m_allocations[name].erase(iter);
          if (m_allocations[name].size() == 0) {
            m_allocations.erase(name);
          }
          m_nallocations--;
          m_availableCores.push_back(mycore);
          break;
        }
      }
      break;
    }
  }
}

void CoreManager::reset() {
  m_allocations.clear();
  m_availableCores.clear();
  m_nallocations = 0;
  m_configured = false;
}

void CoreManager::dump() {
  std::cout << "Dump of CoreManager:" << std::endl;
  std::cout << "Available cores:";
  if (m_availableCores.size() >0) {
    for (auto core: m_availableCores) {
      std::cout << " " << core;
    }
    std::cout << std::endl;
  }
  else {
    std::cout << "<none>" << std::endl;
  }
  std::cout << "Allocation table:" << std::endl;
  if (m_allocations.size() >0) {
    for (auto item: m_allocations) {
      std::cout << " " << item.first;
      for (int core: item.second){
        std::cout << " " << core;
      }
      std::cout << std::endl;
    }
  }
  else {
    std::cout << "  <Empty>" << std::endl;
  }
}
} // namespace dunedaq::coremanager
