cmake_minimum_required(VERSION 3.10)

include(CMakePackageConfigHelpers)

project(LibDataModelInterface LANGUAGES CXX)

set(LIB_NAME DataModelInterface)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TEST "Build the test suits for the library" OFF)

set(DATA_MODEL_INTERFACE_NAME "datamodelinterface")
set(DATA_MODEL_INTERFACE_EXPORTNAME "datamodelinterfaceTargets")

include(GNUInstallDirs)

set(CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR})
set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}/${LIB_NAME})
set(CMAKE_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME})

#make the status optional header available when built as a module
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(datamodelinterface)

add_library(datamodelinterface::datamodelinterface ALIAS datamodelinterface)

target_sources(datamodelinterface PRIVATE
    property.h property.cpp
    changerecorder.h changerecorder.cpp)

target_include_directories(
          datamodelinterface
	  INTERFACE
	  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
	  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

get_target_property(LIB_SRC datamodelinterface SOURCES)

foreach(FILE IN LISTS LIB_SRC)
	    if(FILE MATCHES "^((.*)/)?([^/]+)\\.h$")
			message("Configuring file ${FILE} for installation in ${CMAKE_INSTALL_INCLUDEDIR}/${CMAKE_MATCH_2}")
			install(FILES ${FILE} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${CMAKE_MATCH_2})
		endif()
endforeach()

if (BUILD_TEST)

include(FetchContent)
FetchContent_Declare(
  googletest
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
  FIND_PACKAGE_ARGS NAMES GTest
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
  data_model_interface_test
  test.cpp
  test_internal/generictestdatastructure.h
  test_internal/generictestdatastructure.cpp
)
target_link_libraries(
  data_model_interface_test
  datamodelinterface
  GTest::gtest_main
)

endif()

install(TARGETS datamodelinterface
          EXPORT ${DATA_MODEL_INTERFACE_EXPORTNAME}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      )


install(
  EXPORT ${DATA_MODEL_INTERFACE_EXPORTNAME}
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/cmake/${DATA_MODEL_INTERFACE_NAME}
  NAMESPACE ${DATA_MODEL_INTERFACE_NAME}::
  FILE ${DATA_MODEL_INTERFACE_NAME}Targets.cmake
  )

configure_package_config_file(
  "cmake/Config.cmake.in"
  "${DATA_MODEL_INTERFACE_NAME}Config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/cmake/${DATA_MODEL_INTERFACE_NAME}
  PATH_VARS
    CMAKE_INSTALL_LIBDIR
  )

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${DATA_MODEL_INTERFACE_NAME}ConfigVersion.cmake"
  VERSION "0.0"
  COMPATIBILITY AnyNewerVersion
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${DATA_MODEL_INTERFACE_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${DATA_MODEL_INTERFACE_NAME}ConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_CMAKEDIR}/cmake/${DATA_MODEL_INTERFACE_NAME} )
