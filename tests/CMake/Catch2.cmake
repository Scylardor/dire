function(ADD_CATCH2_DEPENDENCY DOWNLOAD_PREFIX GIT_VERSION TARGET_NAME SCOPE CATCH_TARGET CXX_STANDARD)
	if(NOT TARGET ${CATCH_TARGET}) # If Catch2 has not been found, try to download it from Github
		include(FetchContent)
		FetchContent_Declare(
			Catch2
			PREFIX ${DOWNLOAD_PREFIX}
			GIT_REPOSITORY https://github.com/catchorg/Catch2.git
			GIT_TAG        ${GIT_VERSION}
			OVERRIDE_FIND_PACKAGE
		)

		# Kind of a hack to force FetchContent to comply with the C++ flags we want.
		# target_compile_options or set_target_properties won't work on ALIAS targets.
		set(CMAKE_CXX_STANDARD_OLD "${CMAKE_CXX_STANDARD}")
		set(CMAKE_CXX_STANDARD_REQUIRED_OLD "${CMAKE_CXX_STANDARD_REQUIRED}")
		set(CMAKE_CXX_STANDARD ${CXX_STANDARD})
		set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

		FetchContent_MakeAvailable(Catch2)

		set(CMAKE_CXX_STANDARD "${CMAKE_CXX_STANDARD_OLD}")
		set(CMAKE_CXX_STANDARD_REQUIRED "${CMAKE_CXX_STANDARD_REQUIRED_OLD}")
	endif()

	target_link_libraries(${TARGET_NAME} PRIVATE ${CATCH_TARGET})
	add_dependencies(${TARGET_NAME} ${CATCH_TARGET})

endfunction()