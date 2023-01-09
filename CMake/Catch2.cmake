function(ADD_CATCH2_DEPENDENCY DOWNLOAD_PREFIX GIT_VERSION TARGET_NAME SCOPE WITH_MAIN)
	if(NOT TARGET Catch2::Catch2) # If Catch2 has not been found, try to download it from Github
		include(FetchContent)
		FetchContent_Declare(
			Catch2
			PREFIX ${DOWNLOAD_PREFIX}
			GIT_REPOSITORY https://github.com/catchorg/Catch2.git
			GIT_TAG        ${GIT_VERSION}
		)

		FetchContent_MakeAvailable(Catch2)
	endif()

	if(WITH_MAIN)
		target_link_libraries(${TARGET_NAME} PRIVATE Catch2::Catch2WithMain)
		add_dependencies(${TARGET_NAME} Catch2::Catch2WithMain)
	else()
		target_link_libraries(${TARGET_NAME} PRIVATE Catch2::Catch2)
		add_dependencies(${TARGET_NAME} Catch2::Catch2)
	endif()

endfunction()