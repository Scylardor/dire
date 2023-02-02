cmake_minimum_required(VERSION 3.12) # HOMEPAGE_URL is not supported until CMake version 3.12. We also need C++20.

set(PROJECT_NAME Dire)
string(TOUPPER ${PROJECT_NAME} UPPER_PROJECT_NAME)

project(${PROJECT_NAME}
  VERSION 0.1
  LANGUAGES CXX
  # TODO: HOMEPAGE_URL "DIRE URL"
  DESCRIPTION "A library for Dynamically Interpreted Reflection Expressions (DIRE)."
)

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
set(${UPPER_PROJECT_NAME}_CPP_STANDARD cxx_std_17 CACHE STRING "Set the standard to compile DIRE with. Use cxx_std_20 to enable C++20 features.")
option(${UPPER_PROJECT_NAME}_BUILD_SHARED_LIB "Type of library DIRE will compile. Use STATIC for archives (.a or .lib on Windows) or SHARED for dynamic (.so or DLL on Windows)." ON)

set(DIRE_SOURCE_DIR ${PROJECT_NAME}/${PROJECT_NAME})
set(DIRE_GENERATED_INCLUDES_DIR generated-includes/Dire)

# Main target creation
# Set the appropriate lib type (courtesy of https://gitlab.kitware.com/cmake/community/-/wikis/doc/tutorials/BuildingWinDLL)
set(${PROJECT_NAME}_SOURCES
	${DIRE_SOURCE_DIR}/Dire.h
	${DIRE_SOURCE_DIR}/DireEnums.h
	${DIRE_SOURCE_DIR}/DireReflectable.h
	${DIRE_SOURCE_DIR}/DireReflectable.cpp
	${DIRE_SOURCE_DIR}/DireProperty.h
	${DIRE_SOURCE_DIR}/DirePropertyMetadata.h
	${DIRE_SOURCE_DIR}/DireSubclass.h
	${DIRE_SOURCE_DIR}/DireReflectableID.h
	${DIRE_SOURCE_DIR}/DireDefines.h.in
	${DIRE_SOURCE_DIR}/Handlers/DireTypeHandlers.h
	${DIRE_SOURCE_DIR}/Handlers/DireArrayDataStructureHandler.h
	${DIRE_SOURCE_DIR}/Handlers/DireArrayDataStructureHandler.inl
	${DIRE_SOURCE_DIR}/Handlers/DireMapDataStructureHandler.h
	${DIRE_SOURCE_DIR}/Handlers/DireMapDataStructureHandler.inl
	${DIRE_SOURCE_DIR}/Handlers/DireEnumDataStructureHandler.h
	${DIRE_SOURCE_DIR}/Serialization/DireSerialization.h
	${DIRE_SOURCE_DIR}/Serialization/DireJSONSerializer.h
	${DIRE_SOURCE_DIR}/Serialization/DireJSONSerializer.cpp
	${DIRE_SOURCE_DIR}/Serialization/DireJSONDeserializer.h
	${DIRE_SOURCE_DIR}/Serialization/DireJSONDeserializer.cpp
	${DIRE_SOURCE_DIR}/Serialization/DireBinarySerializer.h
	${DIRE_SOURCE_DIR}/Serialization/DireBinarySerializer.cpp
	${DIRE_SOURCE_DIR}/Serialization/DireBinaryDeserializer.h
	${DIRE_SOURCE_DIR}/Serialization/DireBinaryDeserializer.cpp
	${DIRE_SOURCE_DIR}/Serialization/DireBinaryHeaders.h
	${DIRE_SOURCE_DIR}/Types/DireTypes.h
	${DIRE_SOURCE_DIR}/Types/DireTypeInfoDatabase.h
	${DIRE_SOURCE_DIR}/Types/DireTypeInfoDatabase.cpp
	${DIRE_SOURCE_DIR}/Types/DireTypeInfo.h
	${DIRE_SOURCE_DIR}/Types/DireTypeInfo.inl
	${DIRE_SOURCE_DIR}/Types/DireTypeInfo.cpp
	${DIRE_SOURCE_DIR}/Utils/DireMacros.h
	${DIRE_SOURCE_DIR}/Utils/DireTypeTraits.h
	${DIRE_SOURCE_DIR}/Utils/DireIntrusiveList.h
	${DIRE_SOURCE_DIR}/Utils/DireIntrusiveList.inl
	${DIRE_SOURCE_DIR}/Utils/DireString.h
	${CMAKE_CURRENT_BINARY_DIR}/${DIRE_GENERATED_INCLUDES_DIR}/DireDefines.h
	${CMAKE_CURRENT_BINARY_DIR}/${DIRE_GENERATED_INCLUDES_DIR}/Dire_Export.h
)

# Create the source groups for source tree with root at CMAKE_CURRENT_SOURCE_DIR.
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${${PROJECT_NAME}_SOURCES})

# Decide on the type of library to build
if (${${UPPER_PROJECT_NAME}_BUILD_SHARED_LIB})
	set(${UPPER_PROJECT_NAME}_LIBRARY_TYPE SHARED)
else()
	set(${UPPER_PROJECT_NAME}_LIBRARY_TYPE STATIC)
endif()

add_library(${PROJECT_NAME} ${${UPPER_PROJECT_NAME}_LIBRARY_TYPE}
	${${PROJECT_NAME}_SOURCES}
)
# Alias target
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
	DEBUG_POSTFIX _d
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR})

# On Windows, set a different name for the import library to not be confused with a static build of the library
# (both have .lib extension)
if (CMAKE_IMPORT_LIBRARY_SUFFIX AND ${${UPPER_PROJECT_NAME}_BUILD_SHARED_LIB})
	set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_NAME ${PROJECT_NAME}_imp)
endif()

# specify the C++ standard
target_compile_features(${PROJECT_NAME} PUBLIC ${${UPPER_PROJECT_NAME}_CPP_STANDARD})

# Add more flags than default CMake
target_compile_options(${PROJECT_NAME} PRIVATE
	$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
		-Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wno-c++98-compat-pedantic -Wno-switch-enum -Wno-exit-time-destructors>
	$<$<CXX_COMPILER_ID:GNU>: -Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wno-non-template-friend>
	$<$<CXX_COMPILER_ID:MSVC>: /WX /W4>)

if(${${UPPER_PROJECT_NAME}_CPP_STANDARD} STREQUAL "cxx_std_20")
	set(DIRE_HAS_CPP20 TRUE)
endif()

# Dynamic Library Setup
include(GenerateExportHeader)
generate_export_header( ${PROJECT_NAME}
	BASE_NAME ${PROJECT_NAME}
	EXPORT_MACRO_NAME ${PROJECT_NAME}_EXPORT
	EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/${DIRE_GENERATED_INCLUDES_DIR}/${PROJECT_NAME}_Export.h
	STATIC_DEFINE ${PROJECT_NAME}_STATIC
)

# Follow the MSVC convention of "hide symbols by default and explicitly export what we want"
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_VISIBILITY_PRESET			hidden
    CXX_VISIBILITY_PRESET		hidden
    VISIBILITY_INLINES_HIDDEN	YES
)
#! Dynamic Library Setup

# RapidJSON dependency setup
if(${UPPER_PROJECT_NAME}_SERIALIZATION_ENABLED AND ${UPPER_PROJECT_NAME}_SERIALIZATION_RAPIDJSON_ENABLED)
	include(CMake/RapidJSON.cmake)
	find_rapidjson_dependency(RapidJSON ${${UPPER_PROJECT_NAME}_SERIALIZATION_RAPIDJSON_VERSION})
	target_link_libraries(${PROJECT_NAME} PUBLIC RapidJSON::RapidJSON)
endif()

# Use a generator expression so that the install interface redirects to the right folder after installation.
target_include_directories(${PROJECT_NAME}
    SYSTEM INTERFACE # Avoid users of the library who have more warnings than us to collect our warnings too (cf. https://www.foonathan.net/2018/10/cmake-warnings/)
        $<INSTALL_INTERFACE:include>
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME};${CMAKE_CURRENT_BINARY_DIR}/${DIRE_GENERATED_INCLUDES_DIR}>"
)
target_include_directories(${PROJECT_NAME}
    PRIVATE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME};${CMAKE_CURRENT_BINARY_DIR}/${DIRE_GENERATED_INCLUDES_DIR}>"
)

# Defines (Warning: do this before configure_file or it won't be setup correctly!)
if(${${UPPER_PROJECT_NAME}_ASSERT} STREQUAL "assert")
	set(${UPPER_PROJECT_NAME}_USE_STANDARD_ASSERT TRUE)
endif()

if(${${UPPER_PROJECT_NAME}_STRING} STREQUAL "std")
	set(${UPPER_PROJECT_NAME}_USE_STANDARD_STRING TRUE)
endif()

# Generate the final defines file and add include directories so that clients can find it
configure_file(${DIRE_SOURCE_DIR}/DireDefines.h.in ${DIRE_GENERATED_INCLUDES_DIR}/DireDefines.h)


# INSTALL INSTRUCTIONS
######################
include(GNUInstallDirs) # defines variables like CMAKE_INSTALL_LIBDIR, etc.
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/Dire)

# Install and packaging commands
install(TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}Targets
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} # MODULEs (probably unnecessary)
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} # Static libraries or Import library on Windows (for DLL)
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # DLL file on Windows
)

# Version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION} # use the version of the most recent project() seen (not CMAKE_PROJECT_VERSION)
    COMPATIBILITY AnyNewerVersion
)

install(EXPORT ${PROJECT_NAME}Targets
	FILE ${PROJECT_NAME}Targets.cmake
	NAMESPACE ${PROJECT_NAME}::
	DESTINATION ${INSTALL_CONFIGDIR}
)

install(FILES
	#"${CMAKE_CURRENT_LIST_DIR}/CMake/FindRapidJSON.cmake" (do not install it for now, uncomment if one day we vendor RapidJSON)
	"${CMAKE_CURRENT_LIST_DIR}/CMake/${PROJECT_NAME}Config.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
	DESTINATION ${INSTALL_CONFIGDIR}
)

# Install public headers
install(DIRECTORY ${DIRE_SOURCE_DIR} DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "*.inl")

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/${DIRE_GENERATED_INCLUDES_DIR}/DireDefines.h
	${CMAKE_CURRENT_BINARY_DIR}/${DIRE_GENERATED_INCLUDES_DIR}/Dire_Export.h
	DESTINATION include)


export(EXPORT ${PROJECT_NAME}Targets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::)

# Export it to CMake Package Registry too
set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)
export(PACKAGE ${PROJECT_NAME})

# UNIT TESTS
############
add_subdirectory(tests)

# DOCUMENTATION
###############
include(CMake/Doxygen.cmake)
add_doxygen(${PROJECT_NAME} README.md docs)
