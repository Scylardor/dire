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
add_catch2_dependency(catch2 ${${PROJECT_NAME}_TESTS_CATCH_VERSION} ${PROJECT_NAME}_UnitTests PRIVATE TRUE)

target_link_libraries(${PROJECT_NAME}_UnitTests PRIVATE ${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME}_UnitTests PRIVATE DIRE_UNIT_TESTS=1)