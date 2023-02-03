string(TOUPPER ${PROJECT_NAME} UPPER_PROJECT_NAME)

option(${UPPER_PROJECT_NAME}_TESTS_ENABLED "Builds the unit tests (for library developers)." ON)
set(${UPPER_PROJECT_NAME}_TESTS_CATCH_VERSION "v3.2.1" CACHE STRING "Version of Catch2 library downloaded from Github (if not found).")

if(${UPPER_PROJECT_NAME}_TESTS_ENABLED)

	add_executable(${PROJECT_NAME}_UnitTests
		BasicTests.cpp
		MacroTests.cpp
		EnumTests.cpp
		FunctionTests.cpp
		PropertyTests.cpp
		ReflectableTests.cpp
		SerializationTests.cpp
		TypeTraitsTests.cpp
		TypeInfoDatabaseTests.cpp
		TestClasses.h
	)
	
	# Alias TARGET
	add_executable(${PROJECT_NAME}::Tests ALIAS ${PROJECT_NAME}_UnitTests)

	# Add more flags than default CMake
	target_compile_options(${PROJECT_NAME}_UnitTests PRIVATE
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
			-Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wno-c++98-compat-pedantic -Wno-switch-enum -Wno-exit-time-destructors>
		$<$<CXX_COMPILER_ID:GNU>: -Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wno-non-template-friend>
		$<$<CXX_COMPILER_ID:MSVC>: /WX /W4>)

	target_link_libraries(${PROJECT_NAME}_UnitTests PRIVATE ${PROJECT_NAME})

	# Only try to copy DLLs if we are on a DLL type of system (ie. Windows) and if we are building shared libs
	if (CMAKE_IMPORT_LIBRARY_SUFFIX AND ${${UPPER_PROJECT_NAME}_BUILD_SHARED_LIB})
		add_custom_command(TARGET ${PROJECT_NAME}_UnitTests POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}_UnitTests> $<TARGET_FILE_DIR:${PROJECT_NAME}_UnitTests>
			COMMAND_EXPAND_LISTS)
	endif()

	# Catch2 dependency setup (Do it AFTER the DLL copy command, or the Catch test run will fail if the DLL has not been copied beforehand!)
	include(CMake/Catch2.cmake)
	add_catch2_dependency(catch2 ${${UPPER_PROJECT_NAME}_TESTS_CATCH_VERSION} ${PROJECT_NAME}_UnitTests PRIVATE Catch2::Catch2WithMain 17) # (force using at least C++17)

	enable_testing()
	add_test(NAME ${PROJECT_NAME}_UnitTests COMMAND ${PROJECT_NAME}_UnitTests)

endif()