function(FIND_RAPIDJSON_DEPENDENCY DOWNLOAD_PREFIX GIT_VERSION)
	find_package(rapidjson QUIET)
	if (NOT TARGET rapidjson) # If RapidJson has not been found, try to download it from Github
		include(ExternalProject)
		ExternalProject_Add(
			rapidjson
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
		
		find_package(rapidjson REQUIRED)
	endif()
endfunction()