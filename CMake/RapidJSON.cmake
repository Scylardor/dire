function(FIND_RAPIDJSON_DEPENDENCY DOWNLOAD_PREFIX GIT_VERSION)
	find_package(RapidJSON CONFIG QUIET)

	# Not found? Try again using our FindRapidJSON.cmake just in case
	if(NOT RapidJSON_FOUND)
		list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
		find_package(RapidJSON CONFIG QUIET)
	endif()

	if(RapidJSON_FOUND AND NOT TARGET RapidJSON::RapidJSON)
    add_library(RapidJSON::RapidJSON INTERFACE IMPORTED)
    set_target_properties(RapidJSON::RapidJSON PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${RapidJSON_INCLUDE_DIRS}"
    )
	return()
elseif(RapidJSON_FOUND)
	return()
endif()

	# Still nothing? Try to download it from Github
	if (NOT TARGET RapidJSON::RapidJSON)
		include(ExternalProject)
		ExternalProject_Add(
			RapidJSON
			PREFIX ${DOWNLOAD_PREFIX}
			GIT_REPOSITORY "https://github.com/Tencent/rapidjson.git"
			GIT_TAG ${GIT_VERSION}
			TIMEOUT 10
			CMAKE_ARGS
				-DRAPIDJSON_BUILD_TESTS=OFF
				-DRAPIDJSON_BUILD_DOC=OFF
				-DRAPIDJSON_BUILD_EXAMPLES=OFF
			CONFIGURE_COMMAND ""
			BUILD_COMMAND ""
			INSTALL_COMMAND ""
			UPDATE_COMMAND ""
		)
	endif()

	# At this point it should have worked!
	find_package(RapidJSON REQUIRED)
endfunction()