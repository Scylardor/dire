cmake_minimum_required(VERSION 3.12) # HOMEPAGE_URL is not supported until CMake version 3.12. We also need C++20.

set(PROJECT_NAME Dire)
string(TOUPPER ${PROJECT_NAME} UPPER_PROJECT_NAME)


project(${PROJECT_NAME}
  VERSION 1.0.0
  LANGUAGES CXX
  # TODO: HOMEPAGE_URL "DIRE URL"
  DESCRIPTION "A library for Dynamically Interpreted Reflection Expressions (DIRE)."
)

# specify the C++ standard
# Options
option(${UPPER_PROJECT_NAME}_SERIALIZATION_ENABLED "Enables the Serialization features of the DIRE library." ON)
option(${UPPER_PROJECT_NAME}_SERIALIZATION_RAPIDJSON_ENABLED "Enables the JSON serialization feature of the DIRE library, using RapidJSON." ON)
set(${UPPER_PROJECT_NAME}_SERIALIZATION_RAPIDJSON_VERSION master CACHE STRING "RapidJSON version that DIRE will try to use if not found on the system.")
set(${UPPER_PROJECT_NAME}_SERIALIZATION_RAPIDJSON_ALLOCATOR "rapidjson::CrtAllocator" CACHE STRING "Allocator passed to RapidJSON for its allocations. Uses RapidJSON's one by default.")
option(${UPPER_PROJECT_NAME}_SERIALIZATION_BINARY_ENABLED "Enables the binary serialization feature of the DIRE library." ON)
option(${UPPER_PROJECT_NAME}_SERIALIZABLE_PROPERTIES_BY_DEFAULT "If true, Dire properties are serializable by default, unless tagged with the NotSerializable attribute. If false, they are not serializable by default, unless tagged with the Serializable attribute." ON)

set(${UPPER_PROJECT_NAME}_DEFAULT_CONSTRUCTOR_INSTANTIATE "If on, default-constructible Reflectable types will automatically register their default constructor as their instantiator function." ON)

set(${UPPER_PROJECT_NAME}_NAMESPACE dire CACHE STRING "Namespace the DIRE library will use (in case of a naming conflict)")
set(${UPPER_PROJECT_NAME}_STRING std CACHE STRING "String type the DIRE library should use for standard dynamic string manipulation (std::string by default).")
set(${UPPER_PROJECT_NAME}_ASSERT assert CACHE STRING "Assert expression Dire should use in case of error")
set(${UPPER_PROJECT_NAME}_ALLOCATOR std::allocator CACHE STRING "Stateless allocator Dire should use to dynamically allocate memory.")
set(${UPPER_PROJECT_NAME}_CPP_STANDARD 17 CACHE STRING "Set the standard to compile DIRE with. Input 20 to use C++20 features.")
set(${UPPER_PROJECT_NAME}_LIBRARY_TYPE SHARED CACHE STRING "Type of library DIRE will compile. Use STATIC for archives (.a or .lib on Windows) or SHARED for dynamic (.so or DLL on Windows).")

set(DIRE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/${PROJECT_NAME})
set(DIRE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME})

# Main target creation
# Set the appropriate lib type (courtesy of https://gitlab.kitware.com/cmake/community/-/wikis/doc/tutorials/BuildingWinDLL)
add_library(${PROJECT_NAME} ${${UPPER_PROJECT_NAME}_LIBRARY_TYPE}
	${DIRE_SOURCE_DIR}/Dire.h
	${DIRE_SOURCE_DIR}/DireMacros.h
	${DIRE_SOURCE_DIR}/DireTypes.h
	${DIRE_SOURCE_DIR}/DireTypeTraits.h
	${DIRE_SOURCE_DIR}/DireIntrusiveList.h
	${DIRE_SOURCE_DIR}/DireIntrusiveList.inl
	${DIRE_SOURCE_DIR}/DireTypeInfo.h
	${DIRE_SOURCE_DIR}/DireTypeInfo.inl
	${DIRE_SOURCE_DIR}/DireTypeInfo.cpp
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
	${DIRE_SOURCE_DIR}/DireReflectable.cpp
	${DIRE_SOURCE_DIR}/DireString.h
	${DIRE_SOURCE_DIR}/DireString.inl
	${DIRE_SOURCE_DIR}/DireTypeInfoDatabase.h
	${DIRE_SOURCE_DIR}/DireTypeInfoDatabase.cpp
	${DIRE_SOURCE_DIR}/DireSingleton.h
	${DIRE_SOURCE_DIR}/DireProperty.h
	${DIRE_SOURCE_DIR}/DireProperty.inl
	${DIRE_SOURCE_DIR}/DirePropertyMetadata.h
	${DIRE_SOURCE_DIR}/DirePropertyMetadata.inl
	${DIRE_SOURCE_DIR}/DireSubclass.h
	${DIRE_SOURCE_DIR}/DireAssert.h
	${DIRE_SOURCE_DIR}/DireReflectableID.h
	${DIRE_SOURCE_DIR}/DireDefines.h.in
	${CMAKE_CURRENT_BINARY_DIR}/DireDefines.h
)

# specify the C++ standard
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD ${${UPPER_PROJECT_NAME}_CPP_STANDARD} CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Dynamic Library Setup
include(GenerateExportHeader)
generate_export_header( ${PROJECT_NAME}
	BASE_NAME ${PROJECT_NAME}
	EXPORT_MACRO_NAME ${PROJECT_NAME}_EXPORT
	EXPORT_FILE_NAME ${PROJECT_NAME}_Export.h
	STATIC_DEFINE ${PROJECT_NAME}_STATIC
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    C_VISIBILITY_PRESET			hidden
    CXX_VISIBILITY_PRESET		hidden
    VISIBILITY_INLINES_HIDDEN	YES
)
#! Dynamic Library Setup

# Conditional Sources

if(${UPPER_PROJECT_NAME}_SERIALIZATION_ENABLED)
	target_sources(${PROJECT_NAME} PRIVATE
		${DIRE_SOURCE_DIR}/DireSerialization.h
	)
endif()
if(${UPPER_PROJECT_NAME}_SERIALIZATION_ENABLED AND ${UPPER_PROJECT_NAME}_SERIALIZATION_RAPIDJSON_ENABLED)
	target_sources(${PROJECT_NAME} PRIVATE
		${DIRE_SOURCE_DIR}/DireJSONSerializer.h
		${DIRE_SOURCE_DIR}/DireJSONDeserializer.h
		${DIRE_SOURCE_DIR}/DireJSONSerializer.cpp
		${DIRE_SOURCE_DIR}/DireJSONDeserializer.cpp
	)
	target_compile_definitions(${PROJECT_NAME} PUBLIC DIRE_RAPIDJSON_ALLOCATOR=${${UPPER_PROJECT_NAME}_SERIALIZATION_RAPIDJSON_ALLOCATOR})
endif()
if(${UPPER_PROJECT_NAME}_SERIALIZATION_ENABLED AND ${UPPER_PROJECT_NAME}_SERIALIZATION_BINARY_ENABLED)
	target_sources(${PROJECT_NAME} PRIVATE
		${DIRE_SOURCE_DIR}/DireBinarySerializer.h
		${DIRE_SOURCE_DIR}/DireBinaryDeserializer.h
		${DIRE_SOURCE_DIR}/DireBinarySerializer.cpp
		${DIRE_SOURCE_DIR}/DireBinaryDeserializer.cpp
	)
endif()

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:${PROJECT_NAME}>
        $<BUILD_INTERFACE:${DIRE_INCLUDE_DIR}>
)

# RapidJSON dependency setup
if(${UPPER_PROJECT_NAME}_SERIALIZATION_ENABLED AND ${UPPER_PROJECT_NAME}_SERIALIZATION_RAPIDJSON_ENABLED)
	include(CMake/Rapidjson.cmake)
	add_rapidjson_dependency(rapidjson ${${UPPER_PROJECT_NAME}_SERIALIZATION_RAPIDJSON_VERSION} ${PROJECT_NAME} PUBLIC)
endif()


# Defines
if(${${UPPER_PROJECT_NAME}_ASSERT} STREQUAL "assert")
	set(${UPPER_PROJECT_NAME}_USE_STANDARD_ASSERT TRUE)
endif()

if(${${UPPER_PROJECT_NAME}_STRING} STREQUAL "std")
	set(${UPPER_PROJECT_NAME}_USE_STANDARD_STRING TRUE)
endif()

configure_file(${DIRE_SOURCE_DIR}/DireDefines.h.in DireDefines.h)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# Unit tests
add_subdirectory(tests)
