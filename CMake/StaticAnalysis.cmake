option(LINT_CPPCHECK "Enable CppCheck static analysis" OFF)
option(LINT_CLANG_TIDY "Enable Clang-Tidy static analysis" OFF)

if (LINT_CPPCHECK)
	find_program(CPPCHECK cppcheck)
	if (CPPCHECK)
		set(CMAKE_CXX_CPPCHECK ${CPPCHECK})

      # Append desired arguments to CppCheck
      list(
        APPEND CMAKE_CXX_CPPCHECK
          # Using the below template will allow jumping to any found error from inside Visual Studio output window by double click
		  # (note: template=vs doesn't include the id, which is useful :( )
          "--template \"{file}({line}): {severity} ({id}): {message}\"" 

          # Only show found errors
          "--quiet"

          # Desired warning level in CppCheck
          "--enable=all"

          # Optional: Specified C++ version
          "--std=c++17"

          # Optional: suppression file stored in same directory as the top level CMake script
          "--suppressions-list=${CMAKE_CURRENT_SOURCE_DIR}/CMake/cppcheck_suppressions.txt"

          # Optional: Use inline suppressions
          "--inline-suppr"

		  "-I${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}"

          # Run CppCheck from the working directory, as specified in the add_custom_target command below
          "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}"
        )

      add_custom_target(CPPCHECK DEPENDS ${PROJECT_NAME}
        COMMAND ${CMAKE_CXX_CPPCHECK}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Static code analysis using ${CPP_CHECK_VERSION}")

        add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD COMMAND ${CMAKE_CXX_CPPCHECK}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	else()
		message(SEND_ERROR "CppCheck executable was not found")
	endif()
endif()

if (LINT_CLANG_TIDY)
	find_program(CLANGTIDY clang-tidy)
	if (CLANGTIDY)
        set(CLANG_TIDY_COMMAND "${CLANGTIDY}")
        set(CLANG_TIDY_COMMAND "${CLANGTIDY}" "checks=-*;modernize-*;-prout")
        message(${CLANG_TIDY_COMMAND})
        set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY "C:/msys64/mingw64/bin/clang-tidy.exe;-checks=*;modernize-*")
	else()
		message(SEND_ERROR "Clang-Tidy executable was not found")
	endif()

endif()