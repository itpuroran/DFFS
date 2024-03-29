project(SSAGES)
cmake_minimum_required(VERSION 2.8.8)

# SSAGES version number.
file(STRINGS "VERSION" SSAGES_VERSION)

# Default type is release
if (NOT CMAKE_BUILD_TYPE)
    if (BUILD_TESTS)
        set(CMAKE_BUILD_TYPE "Debug")
    else (BUILD_TESTS)
        set(CMAKE_BUILD_TYPE "Release")
    endif (BUILD_TESTS)
endif()

string (TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_NAME)

# Set the custom modules path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Resolve dependencies
find_package(Git REQUIRED)
find_package(MPI 1.6 REQUIRED)

# Fetch external dependencies
include(FetchEigen)

# Schema builder.
add_subdirectory(schema)

include_directories(${MPI_INCLUDE_PATH})
include_directories(${MPI_CXX_INCLUDE_PATH})
include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_BINARY_DIR}")
include_directories(${CMAKE_CURRENT_BINARY_DIR}/schema/)
add_definitions(${MPI_CXX_COMPILE_FLAGS})

# General source files
set(SOURCE
    src/Methods/ABF.cpp
    src/Methods/ANN.cpp
    src/Methods/BasisFunc.cpp
    src/Methods/CFF.cpp
    src/Methods/DirectForwardFlux.cpp
    src/Methods/ElasticBand.cpp
    src/Methods/FiniteTempString.cpp
    src/Methods/ForwardFlux.cpp
    src/Methods/Meta.cpp
    src/Methods/Method.cpp
    src/Methods/StringMethod.cpp
    src/Methods/Swarm.cpp
    src/Methods/Umbrella.cpp
#    src/Constraints/Constraint.cpp
#    src/Constraints/COPSSImage.cpp
    src/CVs/CollectiveVariable.cpp
    src/CVs/orderparams/particlesystem.cpp
    src/CVs/orderparams/conncomponents.cpp
    src/CVs/orderparams/diagonalize.cpp
    src/CVs/orderparams/gtensor.cpp
 #   src/CVs/orderparams/ldtool.cpp
    src/CVs/orderparams/opfunctions.cpp
    src/CVs/orderparams/orderparameters.cpp
    src/CVs/orderparams/qdata.cpp
    src/CVs/orderparams/qlmfunctions.cpp
    src/CVs/orderparams/readwrite.cpp
    src/CVs/orderparams/lattice.cpp
    src/CVs/orderparams/getneigh.cpp
    src/CVs/orderparams/neighdata.cpp
    src/Loggers/Logger.cpp
    src/Validator/RequirementLoader.cpp
    src/JSON/jsoncpp.cpp
    src/Utility/PairwiseKernel.cpp
    src/Utility/Basis.cpp
    src/Hook.cpp
    src/ResourceHandler.cpp
    "include/nnet/nnet.cpp"
)

####################### MD ENGINES #########################

# Set variables for supported engines.
set(LAMMPS NO CACHE BOOL "Use LAMMPS simulation engine.")
set(GROMACS NO CACHE BOOL "Use GROMACS simulation engine.")
set(LAMMPS_SRC "" CACHE PATH "LAMMPS source directory.")
set(GROMACS_SRC "" CACHE PATH "GROMACS source directory.")
set(QBOX NO CACHE BOOL "Use QBox simulation engine.")
set(OPENMD_SRC "" CACHE PATH "OpenMD source directory.")
set(HOOMD NO CACHE BOOL "Use HOOMD-blue simulation engine.")
set(HOOMD_ROOT "" CACHE PATH "HOOMD-blue installation directory.")

# Determine which simulation engine to use.
if(LAMMPS_SRC OR LAMMPS)
    add_subdirectory(hooks/lammps/)
elseif(GROMACS OR GROMACS_SRC)
    add_subdirectory(hooks/gromacs/)
elseif(QBOX)
    add_subdirectory(hooks/qbox/)
elseif(OPENMD_SRC)
	add_subdirectory(hooks/openmd/)
elseif(HOOMD OR HOOMD_ROOT)
	add_subdirectory(hooks/hoomd/)
else()
    message( FATAL_ERROR "You cannot build SSAGES without specifying an engine." )
endif()

################### END MD ENGINES ######################

# Configure header.
configure_file (
    "${PROJECT_SOURCE_DIR}/src/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
)

# SSAGES library.
add_library(libssages ${HOOKSRC} ${SOURCE})
set_target_properties(libssages PROPERTIES PREFIX "")
target_link_libraries(libssages ${MPI_CXX_LIBRARIES})
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(libssages PROPERTIES COMPILE_FLAGS "-Wall -Wextra -fPIC -g")
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set_target_properties(libssages PROPERTIES COMPILE_FLAGS "-O2 -fPIC -g")
else ()
    set_target_properties(libssages PROPERTIES COMPILE_FLAGS "-O3 -fPIC")
endif ()

# Require C++11.
set_property(TARGET libssages PROPERTY CXX_STANDARD 11)
set_property(TARGET libssages PROPERTY CXX_STANDARD_REQUIRED ON)

# SSAGES executable.
add_executable(ssages
    src/main.cpp
    ${DRIVERSRC}
)

# Call MD engine configuration.
CONFIGURE_ENGINE()

# Dependencies
add_dependencies(libssages eigen schema)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(ssages PROPERTIES COMPILE_FLAGS "-Wall -Wextra -g")
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set_target_properties(ssages PROPERTIES COMPILE_FLAGS "-O2 -g")
else ()
    set_target_properties(ssages PROPERTIES COMPILE_FLAGS "-O3")
endif ()

# Configure flags
# - Globals
if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11")

if (SSAGES_PERF)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -march=native")
endif()

# Add doc subdirectory for building the documentation
add_subdirectory(doc)

if (BUILD_TESTS)
    message(STATUS "")

    # Find Google Test Framework
    include(ExternalProject)

    # Set default ExternalProject root directory
    set_directory_properties(PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/extern)

    # Add gtest
    # http://stackoverflow.com/questions/9689183/cmake-googletest
    ExternalProject_Add(
        googletest
        DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.8.0
        TIMEOUT 600
        CMAKE_ARGS
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        # # Force separate output paths for debug and release builds to allow easy
        # # identification of correct lib in subsequent TARGET_LINK_LIBRARIES commands
        # CMAKE_ARGS -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=DebugLibs
        # -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=ReleaseLibs
        # -Dgtest_force_shared_crt=ON
        # Disable install step
        INSTALL_COMMAND ""
        # Wrap download, configure and build steps in a script to log output
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON
    )

    # Specify include dir
    ExternalProject_Get_Property(googletest source_dir)
    set(GTEST_INCLUDE_DIR ${source_dir}/googletest/include)

    ExternalProject_Get_Property(googletest binary_dir)
    set(GTEST_LIBRARY_PATH ${binary_dir}/googlemock/gtest/${CMAKE_FIND_LIBRARY_PREFIXES}gtest.a)
    set(GTEST_LIBRARY_MAIN_PATH ${binary_dir}/googlemock/gtest/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main.a)
    set(GTEST_LIBRARY gtest)
    add_library(${GTEST_LIBRARY} UNKNOWN IMPORTED)
    set_property(TARGET ${GTEST_LIBRARY} PROPERTY IMPORTED_LOCATION
    ${GTEST_LIBRARY_PATH} )
    add_dependencies(${GTEST_LIBRARY} googletest)

    message(STATUS "")
    message(STATUS "*** Google Test Framework will be used for unit tests")
    message(STATUS "*** GTEST_LIBRARY_PATH = ${GTEST_LIBRARY_PATH}")
    message(STATUS "*** GTEST_INCLUDE_DIR  = ${GTEST_INCLUDE_DIR}")
    message(STATUS "")
    # End gtest

    enable_testing()
    add_subdirectory(test)
endif (BUILD_TESTS)
