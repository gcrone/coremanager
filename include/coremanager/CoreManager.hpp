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

#include "ers/ers.hpp"

namespace dunedaq {

ERS_DECLARE_ISSUE(coremanager,
                  AllocationFailed,
                  "insufficient reservable cores to satisfy request",
  )
ERS_DECLARE_ISSUE(coremanager,
                  AffinitysettingFailed,
                  "Failed to set affinity to requested cpu mask",
  )

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
    void allocate(const std::string& name, int ncores);
    void reset();
    void dump();
    int available() const {return m_availableCores.size();}
    int allocated() const {return m_nallocations;}
  private:
    CoreManager() : m_configured(false) {}

    static std::shared_ptr<CoreManager> s_instance;
    std::vector<int> m_availableCores;
    std::map<std::string,std::vector<int>> m_allocations;
    int m_nallocations;
    bool m_configured;
  };

} // namespace coremanager
} // namespace dunedaq

#endif // COREMANAGER_INCLUDE_COREMANAGER_COREMANAGER_HPP_
