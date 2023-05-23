

# This is a skeleton CMakeLists.txt file, auto-generated on
# Tue Apr 4 16:54:15 BST 2023.  The developer(s) of this package should delete
# this comment as well as adding dependent targets, packages,
# etc. specific to the package. For details on how to write a package,
# please see
# https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-cmake/

cmake_minimum_required(VERSION 3.12)
project(coremanager VERSION 0.0.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()


find_package(Boost COMPONENTS unit_test_framework REQUIRED)

find_package(ers REQUIRED)
find_package(logging REQUIRED)
find_package(coredal REQUIRED)

##############################################################################


daq_add_library(CoreManager.cpp LINK_LIBRARIES ers::ers logging::logging
  coredal::coredal)

##############################################################################

daq_add_unit_test(CoreManager_test LINK_LIBRARIES coremanager coredal::coredal ers::ers)

##############################################################################

daq_install()

