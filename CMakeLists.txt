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
option(${PROJECT_NAME}_SERIALIZABLE_PROPERTIES_BY_DEFAULT "If true, Dire properties are serializable by default, unless tagged with the NotSerializable attribute. If false, Dire properties are not serializable by default, unless tagged with the Serializable attribute." ON)

set(${PROJECT_NAME}_NAMESPACE "dire" CACHE STRING "Namespace the DIRE library will use (in case of a naming conflict)")
set(${PROJECT_NAME}_STRING "std::string" CACHE STRING "Type the DIRE library should use for standard dynamic string manipulation.")

option(${PROJECT_NAME}_BUILD_UNIT_TESTS "Builds the unit tests of the DIRE library (for library developers)." OFF)
set(${PROJECT_NAME}_TESTS_CATCH_VERSION "v3.2.1" CACHE STRING "Version of Catch2 library downloaded from Github (for library developers).")


set(DIRE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/${PROJECT_NAME})
set(DIRE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME})

# Main target creation
add_library(${PROJECT_NAME} INTERFACE # Dire is header-only: make it an interface
	${DIRE_SOURCE_DIR}/Dire.h
	${DIRE_SOURCE_DIR}/DireMacros.h
	${DIRE_SOURCE_DIR}/DireTypes.h
	${DIRE_SOURCE_DIR}/DireTypeTraits.h
	${DIRE_SOURCE_DIR}/DireIntrusiveList.h
	${DIRE_SOURCE_DIR}/DireIntrusiveList.inl
	${DIRE_SOURCE_DIR}/DireTypeInfo.h
	${DIRE_SOURCE_DIR}/DireTypeInfo.inl
	${DIRE_SOURCE_DIR}/DireTypeHandlers.h
	${DIRE_SOURCE_DIR}/DireTypeHandlers.inl
	${DIRE_SOURCE_DIR}/DireArrayDataStructureHandler.h
	${DIRE_SOURCE_DIR}/DireArrayDataStructureHandler.inl
	${DIRE_SOURCE_DIR}/DireMapDataStructureHandler.h
	${DIRE_SOURCE_DIR}/DireMapDataStructureHandler.inl
	${DIRE_SOURCE_DIR}/DireEnumDataStructureHandler.h
	${DIRE_SOURCE_DIR}/DireEnums.h
	${DIRE_SOURCE_DIR}/DireEnums.inl
	${DIRE_SOURCE_DIR}/DireCRTP.h
	${DIRE_SOURCE_DIR}/DireReflectable.h
	${DIRE_SOURCE_DIR}/DireString.h
	${DIRE_SOURCE_DIR}/DireString.inl
	${DIRE_SOURCE_DIR}/DireTypeInfoDatabase.h
	${DIRE_SOURCE_DIR}/DireSingleton.h
	${DIRE_SOURCE_DIR}/DireStaticTypeCounter.h
	${DIRE_SOURCE_DIR}/DireProperty.h
	${DIRE_SOURCE_DIR}/DireProperty.inl
	${DIRE_SOURCE_DIR}/DirePropertyMetadata.h
	${DIRE_SOURCE_DIR}/DirePropertyMetadata.inl
)

# Conditional Sources

# CMake seems to be a bit weird in how they manage sources for INTERFACE libraries.
# Declaring the target with an INTERFACE list of sources puts them in the lib, but adding more sources with INTERFACE scope propagates those in user targets, not in the lib target.
# Fortunately, declaring them PRIVATE instead seems to work.
if(${PROJECT_NAME}_SERIALIZATION_ENABLED)
	target_sources(${PROJECT_NAME} PRIVATE
		${DIRE_SOURCE_DIR}/DireSerialization.h
	)
endif()
if(${PROJECT_NAME}_SERIALIZATION_ENABLED AND ${PROJECT_NAME}_SERIALIZATION_JSON_SERIALIZER_ENABLED)
	target_sources(${PROJECT_NAME} PRIVATE
		${DIRE_SOURCE_DIR}/DireJSONSerializer.h
		${DIRE_SOURCE_DIR}/DireJSONDeserializer.h
	)
endif()
if(${PROJECT_NAME}_SERIALIZATION_ENABLED AND ${PROJECT_NAME}_SERIALIZATION_BINARY_SERIALIZER_ENABLED)
	target_sources(${PROJECT_NAME} PRIVATE
		${DIRE_SOURCE_DIR}/DireBinarySerializer.h
		${DIRE_SOURCE_DIR}/DireBinaryDeserializer.h
	)
endif()

target_include_directories(${PROJECT_NAME}
    INTERFACE # Need to use INTERFACE scope for an INTERFACE target.
        $<INSTALL_INTERFACE:${PROJECT_NAME}>
        $<BUILD_INTERFACE:${DIRE_INCLUDE_DIR}>
)

# RapidJSON dependency setup
if(${PROJECT_NAME}_SERIALIZATION_ENABLED AND ${PROJECT_NAME}_USE_JSON_SERIALIZER)
	include(CMake/Rapidjson.cmake)
	# Need to use INTERFACE scope for an INTERFACE target.
	add_rapidjson_dependency(rapidjson master ${PROJECT_NAME} INTERFACE)
endif()


# Defines
target_compile_definitions(${PROJECT_NAME} INTERFACE DIRE_NS=${${PROJECT_NAME}_NAMESPACE})
target_compile_definitions(${PROJECT_NAME} INTERFACE DIRE_USE_STD_STRING=1)
if(${${PROJECT_NAME}_STRING} STREQUAL "std::string")
	target_compile_definitions(${PROJECT_NAME} INTERFACE DIRE_USE_STD_STRING=1)
else()
	target_compile_definitions(${PROJECT_NAME} INTERFACE DIRE_STRING=${${PROJECT_NAME}_STRING})
endif()
if(${PROJECT_NAME}_SERIALIZATION_ENABLED)
	target_compile_definitions(${PROJECT_NAME} INTERFACE DIRE_USE_SERIALIZATION=1)
endif()
if(${PROJECT_NAME}_SERIALIZATION_ENABLED AND ${PROJECT_NAME}_SERIALIZATION_JSON_SERIALIZER_ENABLED)
	target_compile_definitions(${PROJECT_NAME} INTERFACE DIRE_USE_JSON_SERIALIZATION=1)
endif()
if(${PROJECT_NAME}_SERIALIZATION_ENABLED AND ${PROJECT_NAME}_SERIALIZATION_BINARY_SERIALIZER_ENABLED)
	target_compile_definitions(${PROJECT_NAME} INTERFACE DIRE_USE_BINARY_SERIALIZATION=1)
endif()
if(${PROJECT_NAME}_SERIALIZATION_ENABLED AND ${PROJECT_NAME}_SERIALIZABLE_PROPERTIES_BY_DEFAULT)
	target_compile_definitions(${PROJECT_NAME} INTERFACE DIRE_SERIALIZABLE_PROPERTIES_BY_DEFAULT=1)
endif()

# Unit tests
if(${PROJECT_NAME}_BUILD_UNIT_TESTS)
	add_executable(${PROJECT_NAME}_UnitTests
		tests/BasicTests.cpp
		tests/MacroTests.cpp
	)

	# Catch2 dependency setup
	include(CMake/Catch2.cmake)
	add_catch2_dependency(catch2 ${${PROJECT_NAME}_TESTS_CATCH_VERSION} ${PROJECT_NAME}_UnitTests PRIVATE TRUE)

	target_link_libraries(${PROJECT_NAME}_UnitTests PRIVATE ${PROJECT_NAME})
endif()

