string(TOUPPER ${PROJECT_NAME} UPPER_PROJECT_NAME)

option(${UPPER_PROJECT_NAME}_TESTS_ENABLED "Builds the unit tests (for library developers)." OFF)
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

	# specify the C++ standard
	set_target_properties(${PROJECT_NAME}_UnitTests PROPERTIES CXX_STANDARD ${${UPPER_PROJECT_NAME}_CPP_STANDARD} CMAKE_CXX_STANDARD_REQUIRED TRUE)

	# Enable CTest
	include(CTest)
	enable_testing()

	# Catch2 dependency setup
	include(CMake/Catch2.cmake)
	add_catch2_dependency(catch2 ${${UPPER_PROJECT_NAME}_TESTS_CATCH_VERSION} ${PROJECT_NAME}_UnitTests PRIVATE Catch2::Catch2WithMain ${${UPPER_PROJECT_NAME}_CPP_STANDARD})
	find_package(Catch2 REQUIRED)
	# Needed for catch_discover_tests to work in case we used FetchContent
	# (cf. https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#catchcmake-and-catchaddtestscmake )
	list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
	include(Catch)
	catch_discover_tests(${PROJECT_NAME}_UnitTests)

	target_link_libraries(${PROJECT_NAME}_UnitTests PRIVATE ${PROJECT_NAME})

	set(UNIT_TESTS_DEFINE ${UPPER_PROJECT_NAME}_UNIT_TESTS )

	target_compile_definitions(${PROJECT_NAME}_UnitTests PRIVATE ${UNIT_TESTS_DEFINE}=1)

	if (${UPPER_PROJECT_NAME}_BUILD_SHARED_LIB)
		add_custom_command(TARGET ${PROJECT_NAME}_UnitTests POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}_UnitTests> $<TARGET_FILE_DIR:${PROJECT_NAME}_UnitTests>
			COMMAND_EXPAND_LISTS
		)
	endif()

endif()