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

	# Catch2 dependency setup
	include(CMake/Catch2.cmake)
	add_catch2_dependency(catch2 ${${UPPER_PROJECT_NAME}_TESTS_CATCH_VERSION} ${PROJECT_NAME}_UnitTests PRIVATE TRUE)

	target_link_libraries(${PROJECT_NAME}_UnitTests PRIVATE ${PROJECT_NAME})

	set(UNIT_TESTS_DEFINE ${UPPER_PROJECT_NAME}_UNIT_TESTS )

	target_compile_definitions(${PROJECT_NAME}_UnitTests PRIVATE ${UNIT_TESTS_DEFINE}=1)

endif()