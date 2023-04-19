/**
 * @file CoreManager.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "logging/Logging.hpp"

#include "coremanager/CoreManager.hpp"

#include <cstring>
#include <memory>
#include <regex>
#include <iostream>

#include <pthread.h>

namespace dunedaq::coremanager {
  std::shared_ptr<CoreManager> CoreManager::s_instance = nullptr;

void CoreManager::configure(const std::string& corelist) {
  // Make sure we start from a known position
  reset();

  static const std::regex re(
    R"(((\d+)-(\d+)(:(\d))?)|((\d+)\.\.(\d+)(:(\d))?)|\d+)");
  std::smatch sm;
  for (std::string s(corelist); std::regex_search(s, sm, re); s = sm.suffix()) {
    int start;
    int end;
    int step = 1;
    if (sm[2].length()) {
      // n-m
      start = std::stoi(sm[2]);
      end = std::stoi(sm[3]);
      if (sm[5].length()) {
        step = std::stoi(sm[5]);
      }
    }
    else if (sm[7].length()) {
      // n..m
      start = std::stoi(sm[7]);
      end = std::stoi(sm[8]);
      if (sm[10].length()) {
        step = std::stoi(sm[10]);
      }
    }
    else {
      start = std::stoi(sm[0]);
      end = std::stoi(sm[0]);
      step = 1;
    }
    for (int i = start; i <= end; i += step) {
      m_availableCores.push_back(i);
    }
  }

  // Set our processor mask to all available cores
  CPU_ZERO(&m_pmask);
  for (auto core : m_availableCores) {
    CPU_SET(core, &m_pmask);
  }

  TLOG() << "Now have " << CPU_COUNT(&m_pmask) << " cores for general use";
  // And set affinity to that mask
  auto status=sched_setaffinity(m_pid, sizeof(m_pmask), &m_pmask);
  if (status != 0) {
    throw (AffinitySettingFailed(ERS_HERE, std::strerror(errno)));
  }

  m_configured = true;
}

void CoreManager::allocate(const std::string& name, unsigned int ncores) {
  TLOG() << "Reserving " << ncores << " cores for " << name;
  if (ncores > 0) {
    if (m_availableCores.size() >= ncores) {
      if (m_allocations.find(name) == m_allocations.end()) {
        m_allocations[name] = std::vector<int>();
      }
      for (unsigned int i=0; i<ncores; ++i) {
        auto core = m_availableCores.back();
        m_allocations[name].push_back(core);
        m_availableCores.pop_back();
        m_nallocations++;
        CPU_CLR(core, &m_pmask);
      }
      TLOG() << "Now have " << CPU_COUNT(&m_pmask) << " cores for general use";
      auto status = sched_setaffinity(m_pid, sizeof(m_pmask), &m_pmask);
      if (status != 0) {
        throw (AffinitySettingFailed(ERS_HERE,std::strerror(errno)));
      }
    }
    else {
      // throw an ers issue
      throw (AllocationFailed(ERS_HERE));
    }
  }
}

void CoreManager::setAffinity(const std::string& name) {
  TLOG() << "Setting affinity for " << name
         << " (thread " << pthread_self() << ")";
  if (m_allocations.find(name) != m_allocations.end()) {
    cpu_set_t pmask;
    CPU_ZERO(&pmask);
    for (auto core : m_allocations[name]) {
      CPU_SET(core, &pmask);
    }
    int status = sched_setaffinity(0, sizeof(pmask), &pmask);
    if (status != 0) {
      throw (AffinitySettingFailed(ERS_HERE,std::strerror(errno)));
    }
  }
  else {
    auto status = sched_setaffinity(0, sizeof(m_pmask), &m_pmask);
    if (status != 0) {
      throw (AffinitySettingFailed(ERS_HERE,std::strerror(errno)));
    }
  }
}

void CoreManager::release(const std::string& name) {
  if (m_allocations.find(name) != m_allocations.end()) {
    cpu_set_t pmask;
    CPU_ZERO(&pmask);
    for (auto core : m_allocations[name]) {
      CPU_SET(core, &pmask);
      m_nallocations--;
      m_availableCores.push_back(core);
    }
    m_allocations.erase(name);

    // Put the cores we're releasing back into the process general CPU set
    CPU_OR(&m_pmask, &m_pmask, &pmask);
    TLOG() << "Now have " << CPU_COUNT(&m_pmask) << " cores for general use";
    int status = sched_setaffinity(m_pid, sizeof(m_pmask), &m_pmask);
    if (status != 0) {
      throw (AffinitySettingFailed(ERS_HERE,std::strerror(errno)));
    }
  }

  int status = sched_setaffinity(0, sizeof(m_pmask), &m_pmask);
  if (status != 0) {
    throw (AffinitySettingFailed(ERS_HERE,std::strerror(errno)));
  }
}

void CoreManager::reset() {
  m_allocations.clear();
  m_availableCores.clear();
  m_nallocations = 0;
  m_configured = false;
}

std::string CoreManager::affinityString() {
  cpu_set_t pmask;
  auto status = sched_getaffinity(0, sizeof(pmask), &pmask);
  if (status != 0) {
    throw (AffinityGettingFailed(ERS_HERE));
  }
  std::ostringstream stream;
  auto ncores = CPU_COUNT(&pmask);
  stream << "Affinity currently set to " << ncores
         << " core" << (ncores>1 ? "s:" : ":");
  for (int core = 0; core < CPU_SETSIZE; core++) {
    if (CPU_ISSET(core, &pmask)) {
      stream << " " << core;
    }
  }
  return stream.str();
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
