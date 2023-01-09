function(ADD_RAPIDJSON_DEPENDENCY DOWNLOAD_PREFIX GIT_VERSION TARGET_NAME SCOPE)
	if(RapidJSON_FOUND OR TARGET rapidjson)
		set(RAPIDJSON_INCLUDE_DIR ${RAPIDJSON_INCLUDE_DIRS})
	else() # If RapidJson has not been found, try to download it from Github
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
		# Assume RapidJSON's include dir will be "include"
		ExternalProject_Get_Property(rapidjson SOURCE_DIR)
		set(RAPIDJSON_INCLUDE_DIR "${SOURCE_DIR}/include")
	endif()

	target_include_directories(${TARGET_NAME} ${SCOPE} ${RAPIDJSON_INCLUDE_DIR})
	add_dependencies(${TARGET_NAME} rapidjson)
endfunction()

function(FETCH_RAPIDJSON_DEPENDENCY DOWNLOAD_PREFIX GIT_VERSION TARGET_NAME SCOPE)
	if(RapidJSON_FOUND)
		set(RAPIDJSON_INCLUDE_DIR ${RAPIDJSON_INCLUDE_DIRS})
	else() # If RapidJson has not been found, try to download it from Github
		include(FetchContent)
		FetchContent_Declare(
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
		# Assume RapidJSON's include dir will be "include"
		FetchContent_MakeAvailable(rapidjson)
		set(RAPIDJSON_INCLUDE_DIR "${rapidjson_SOURCE_DIR}/include")
	endif()

	target_include_directories(${TARGET_NAME} ${SCOPE} ${RAPIDJSON_INCLUDE_DIR})
	add_dependencies(${TARGET_NAME} rapidjson)
endfunction()