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
option(${PROJECT_NAME}_SERIALIZATION_RAPIDJSON_ENABLED "Enables the JSON serialization feature of the DIRE library, using RapidJSON." ON)
set(${PROJECT_NAME}_SERIALIZATION_RAPIDJSON_VERSION master CACHE STRING "RapidJSON version that DIRE will try to use if not found on the system.")
set(${PROJECT_NAME}_SERIALIZATION_RAPIDJSON_ALLOCATOR "rapidjson::CrtAllocator" CACHE STRING "Allocator passed to RapidJSON for its allocations. Uses RapidJSON's one by default.")
option(${PROJECT_NAME}_SERIALIZATION_BINARY_ENABLED "Enables the binary serialization feature of the DIRE library." ON)
option(${PROJECT_NAME}_SERIALIZABLE_PROPERTIES_BY_DEFAULT "If true, Dire properties are serializable by default, unless tagged with the NotSerializable attribute. If false, they are not serializable by default, unless tagged with the Serializable attribute." ON)

set(${PROJECT_NAME}_REFLECTABLE_INSTANTIATE_WITH_DEFAULT_CONSTRUCTOR "If on, default-constructible Reflectable types will automatically register their default constructor as their instantiator function." ON)

set(${PROJECT_NAME}_NAMESPACE "dire" CACHE STRING "Namespace the DIRE library will use (in case of a naming conflict)")
set(${PROJECT_NAME}_STRING "std::string" CACHE STRING "Type the DIRE library should use for standard dynamic string manipulation.")
set(${PROJECT_NAME}_ASSERT "assert" CACHE STRING "Assert expression Dire should use in case of error")



set(DIRE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/${PROJECT_NAME})
set(DIRE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/${PROJECT_NAME})

# Main target creation
add_library(${PROJECT_NAME} STATIC #TODO customize
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
	${DIRE_SOURCE_DIR}/DireStaticTypeCounter.h
	${DIRE_SOURCE_DIR}/DireProperty.h
	${DIRE_SOURCE_DIR}/DireProperty.inl
	${DIRE_SOURCE_DIR}/DirePropertyMetadata.h
	${DIRE_SOURCE_DIR}/DirePropertyMetadata.inl
	${DIRE_SOURCE_DIR}/DireSubclass.h
	${DIRE_SOURCE_DIR}/DireAssert.h
	${DIRE_SOURCE_DIR}/DireReflectableID.h
)

# Conditional Sources

if(${PROJECT_NAME}_SERIALIZATION_ENABLED)
	target_sources(${PROJECT_NAME} PRIVATE
		${DIRE_SOURCE_DIR}/DireSerialization.h
	)
endif()
if(${PROJECT_NAME}_SERIALIZATION_ENABLED AND ${PROJECT_NAME}_SERIALIZATION_RAPIDJSON_ENABLED)
	target_sources(${PROJECT_NAME} PRIVATE
		${DIRE_SOURCE_DIR}/DireJSONSerializer.h
		${DIRE_SOURCE_DIR}/DireJSONDeserializer.h
		${DIRE_SOURCE_DIR}/DireJSONSerializer.cpp
		${DIRE_SOURCE_DIR}/DireJSONDeserializer.cpp
	)
	target_compile_definitions(${PROJECT_NAME} PUBLIC DIRE_RAPIDJSON_ALLOCATOR=${${PROJECT_NAME}_SERIALIZATION_RAPIDJSON_ALLOCATOR})
endif()
if(${PROJECT_NAME}_SERIALIZATION_ENABLED AND ${PROJECT_NAME}_SERIALIZATION_BINARY_ENABLED)
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
if(${PROJECT_NAME}_SERIALIZATION_ENABLED AND ${PROJECT_NAME}_SERIALIZATION_RAPIDJSON_ENABLED)
	include(CMake/Rapidjson.cmake)
	add_rapidjson_dependency(rapidjson ${${PROJECT_NAME}_SERIALIZATION_RAPIDJSON_VERSION} ${PROJECT_NAME} PUBLIC)
endif()


# Defines

# Disable MSVC "unsafe" functions warnings
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

target_compile_definitions(${PROJECT_NAME} PUBLIC DIRE_NS=${${PROJECT_NAME}_NAMESPACE})

if(${${PROJECT_NAME}_STRING} STREQUAL "std::string")
	target_compile_definitions(${PROJECT_NAME} PUBLIC DIRE_USE_STD_STRING=1)
else()
	target_compile_definitions(${PROJECT_NAME} PUBLIC DIRE_STRING=${${PROJECT_NAME}_STRING})
endif()

if(${${PROJECT_NAME}_ASSERT} STREQUAL "assert")
	target_compile_definitions(${PROJECT_NAME} PUBLIC DIRE_USE_STD_ASSERT=1)
else()
	target_compile_definitions(${PROJECT_NAME} PUBLIC DIRE_ASSERT=${${PROJECT_NAME}_ASSERT})
endif()

if(${PROJECT_NAME}_REFLECTABLE_INSTANTIATE_WITH_DEFAULT_CONSTRUCTOR)
	target_compile_definitions(${PROJECT_NAME} PUBLIC DIRE_DEFAULT_CONSTRUCTOR_INSTANTIATE=1)
endif()

if(${PROJECT_NAME}_SERIALIZATION_ENABLED)
	target_compile_definitions(${PROJECT_NAME} PUBLIC DIRE_USE_SERIALIZATION=1)
endif()

if(${PROJECT_NAME}_SERIALIZATION_ENABLED AND ${PROJECT_NAME}_SERIALIZATION_RAPIDJSON_ENABLED)
	target_compile_definitions(${PROJECT_NAME} PUBLIC DIRE_USE_JSON_SERIALIZATION=1)
endif()

if(${PROJECT_NAME}_SERIALIZATION_ENABLED AND ${PROJECT_NAME}_SERIALIZATION_BINARY_ENABLED)
	target_compile_definitions(${PROJECT_NAME} PUBLIC DIRE_USE_BINARY_SERIALIZATION=1)
endif()

if(${PROJECT_NAME}_SERIALIZATION_ENABLED AND ${PROJECT_NAME}_SERIALIZABLE_PROPERTIES_BY_DEFAULT)
	target_compile_definitions(${PROJECT_NAME} PUBLIC DIRE_SERIALIZABLE_PROPERTIES_BY_DEFAULT=1)
endif()

# Unit tests
add_subdirectory(tests)



