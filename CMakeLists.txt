cmake_minimum_required(VERSION 3.12) # HOMEPAGE_URL is not supported until CMake version 3.12

set(PROJECT_NAME Dire)

project(${PROJECT_NAME}
  VERSION 1.0.0
  LANGUAGES CXX
  # TODO: HOMEPAGE_URL "DIRE URL" 
  DESCRIPTION "A library for Dynamically Interpreted Reflection Expressions (DIRE)."
)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Options
option(${PROJECT_NAME}_SERIALIZATION_ENABLED "Enables the Serialization features of the DIRE library." ON)
option(${PROJECT_NAME}_SERIALIZATION_JSON_SERIALIZER_ENABLED "Enables the JSON serialization feature of the DIRE library, using RapidJSON." ON)
set(${PROJECT_NAME}_SERIALIZATION_JSON_SERIALIZER_VERSION "master" CACHE STRING "RapidJSON version that DIRE will try to use if not found on the system.")
option(${PROJECT_NAME}_SERIALIZATION_BINARY_SERIALIZER_ENABLED "Enables the binary serialization feature of the DIRE library." ON)

set(${PROJECT_NAME}_NAMESPACE "dire" CACHE STRING "Namespace the DIRE library will use (in case of a naming conflict)")

option(${PROJECT_NAME}_BUILD_UNIT_TESTS "Builds the unit tests of the DIRE library (for library developers)." OFF)
set(${PROJECT_NAME}_TESTS_CATCH_VERSION "v3.2.1" CACHE STRING "Version of Catch2 library downloaded from Github (for library developers).")


set(DIRE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/${PROJECT_NAME})
set(DIRE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME})

# Main target creation
add_library(${PROJECT_NAME} INTERFACE # Dire is header-only: make it an interface
	${DIRE_SOURCE_DIR}/DireMacros.h
	${DIRE_SOURCE_DIR}/Dire.h
)

target_include_directories(${PROJECT_NAME}
    INTERFACE # Need to use INTERFACE scope for an INTERFACE target.
        $<INSTALL_INTERFACE:${PROJECT_NAME}>
        $<BUILD_INTERFACE:${DIRE_INCLUDE_DIR}>
)

if(${PROJECT_NAME}_SERIALIZATION_ENABLED)
	target_sources(${PROJECT_NAME} INTERFACE
		${DIRE_SOURCE_DIR}/DireSerialization.h
	)
endif()

if(${PROJECT_NAME}_SERIALIZATION_ENABLED AND ${PROJECT_NAME}_SERIALIZATION_JSON_SERIALIZER_ENABLED)
	target_sources(${PROJECT_NAME} INTERFACE
		${DIRE_SOURCE_DIR}/DireJSONSerializer.h
		${DIRE_SOURCE_DIR}/DireJSONDeserializer.h	
	)
endif()

if(${PROJECT_NAME}_SERIALIZATION_ENABLED AND ${PROJECT_NAME}_SERIALIZATION_BINARY_SERIALIZER_ENABLED)
	target_sources(${PROJECT_NAME} INTERFACE
		${DIRE_SOURCE_DIR}/DireBinarySerializer.h
		${DIRE_SOURCE_DIR}/DireBinaryDeserializer.h
	)
endif()

# RapidJSON dependency setup
if(${PROJECT_NAME}_SERIALIZATION_ENABLED AND ${PROJECT_NAME}_USE_JSON_SERIALIZER)
	include(CMake/Rapidjson.cmake)
	# Need to use INTERFACE scope for an INTERFACE target.
	add_rapidjson_dependency(rapidjson master ${PROJECT_NAME} INTERFACE)
endif()

# Catch2 dependency setup
if(${PROJECT_NAME}_BUILD_UNIT_TESTS)
	add_executable(${PROJECT_NAME}_UnitTests
		tests/BasicTests.cpp
		tests/MacroTests.cpp
	)
	
	include(CMake/Catch2.cmake)
	add_catch2_dependency(catch2 ${${PROJECT_NAME}_TESTS_CATCH_VERSION} ${PROJECT_NAME}_UnitTests PRIVATE TRUE)
	
	target_link_libraries(${PROJECT_NAME}_UnitTests PRIVATE ${PROJECT_NAME})
endif()






