/**
 * @file CoreManager.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef COREMANAGER_INCLUDE_COREMANAGER_COREMANAGER_HPP_
#define COREMANAGER_INCLUDE_COREMANAGER_COREMANAGER_HPP_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <unistd.h>

#include "ers/ers.hpp"

namespace dunedaq {

ERS_DECLARE_ISSUE(coremanager,
                  AllocationFailed,
                  "insufficient reservable cores to satisfy request",
                  ERS_EMPTY)
ERS_DECLARE_ISSUE(coremanager,
                  AffinitySettingFailed,
                  "Failed to set affinity to requested cpu mask, " << reason,
                  ((std::string) reason))
ERS_DECLARE_ISSUE(coremanager,
                  AffinityGettingFailed,
                  "Failed to get affinity for current thread",
                  ERS_EMPTY)
ERS_DECLARE_ISSUE(coremanager,
                  AffinityNotSet,
                  "Release failed, no affinity set for current thread",
                  ERS_EMPTY)

namespace coremanager {

  class CoreManager {

public:
  static std::shared_ptr<CoreManager> get()
  {
    if (!s_instance) {
      s_instance = std::shared_ptr<CoreManager>(new CoreManager());
    }
    return s_instance;
  }

  CoreManager(const CoreManager&) = delete;            ///< CoreManager is not copy-constructible
  CoreManager& operator=(const CoreManager&) = delete; ///< CoreManager is not copy-assignable
  CoreManager(CoreManager&&) = delete;                 ///< CoreManager is not move-constructible
  CoreManager& operator=(CoreManager&&) = delete;      ///< CoreManager is not move-assignable

    void configure(const std::string& corelist);
    void allocate(const std::string& name, unsigned int ncores);
    void setAffinity(const std::string& name);
    void release(const std::string& name);
    void reset();
    void dump();
    unsigned int available() const {return m_availableCores.size();}
    unsigned int allocated() const {return m_nallocations;}
    std::string affinityString();
  private:
    CoreManager() : m_pid(getpid()), m_nallocations(0), m_configured(false) {}

    static std::shared_ptr<CoreManager> s_instance;
    pid_t m_pid;
    cpu_set_t m_pmask;
    std::vector<int> m_availableCores;
    std::map<std::string,std::vector<int>> m_allocations;
    unsigned int m_nallocations;
    bool m_configured;
  };

} // namespace coremanager
} // namespace dunedaq

#endif // COREMANAGER_INCLUDE_COREMANAGER_COREMANAGER_HPP_
