# Locate the RapidJSON library
#
# This module defines the following variables:
#
# RAPIDJSON_INCLUDE_DIR where to find RAPIDJSON include files.
# RAPIDJSON_FOUND true if RAPIDJSON_INCLUDE_DIR has been found.
#
# To help locate the library and include file, you can define a
# variable called RAPIDJSON_ROOT which points to the root of the RAPIDJSON library
# installation.

set( _RAPIDJSON_HEADER_SEARCH_DIRS
"/usr/include"
"/usr/local/include"
"${CMAKE_SOURCE_DIR}/include"
"C:\Program Files (x86)"
)
set( _RAPIDJSON_LIB_SEARCH_DIRS
"/usr/lib"
"/usr/local/lib"
"${CMAKE_SOURCE_DIR}/lib"
"C:\Program Files (x86)")

# Check environment for root search directory
set( _RAPIDJSON_ENV_ROOT $ENV{RAPIDJSON_ROOT} )
if( NOT RAPIDJSON_ROOT AND _RAPIDJSON_ENV_ROOT )
	set(RAPIDJSON_ROOT ${_RAPIDJSON_ENV_ROOT})
endif()

# Put user specified location at beginning of search
if( RAPIDJSON_ROOT )
	list( INSERT _RAPIDJSON_HEADER_SEARCH_DIRS 0 "${RAPIDJSON_ROOT}" )
endif()

# Search for the header
find_path(RapidJSON_INCLUDE_DIR "rapidjson/rapidjson.h"
PATHS ${_RAPIDJSON_HEADER_SEARCH_DIRS} )


INCLUDE(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RapidJSON DEFAULT_MSG RapidJSON_INCLUDE_DIR)

if(RapidJSON_FOUND AND NOT TARGET RapidJSON::RapidJSON)
    add_library(RapidJSON::RapidJSON INTERFACE IMPORTED)
    set_target_properties(RapidJSON::RapidJSON PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${RapidJSON_INCLUDE_DIRS}"
    )
endif()
