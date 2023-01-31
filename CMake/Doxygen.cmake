macro(UseDoxygenAwesomeCss)
  include(FetchContent)
  FetchContent_Declare(doxygen-awesome-css
    GIT_REPOSITORY
      https://github.com/jothepro/doxygen-awesome-css.git
    GIT_TAG
      v1.6.0
  )
  FetchContent_MakeAvailable(doxygen-awesome-css)
  
  set(DOXYGEN_GENERATE_TREEVIEW     YES)
  set(DOXYGEN_HAVE_DOT              YES)
  set(DOXYGEN_DOT_IMAGE_FORMAT      svg)
  set(DOXYGEN_DOT_TRANSPARENT       YES)
  set(DOXYGEN_HTML_EXTRA_STYLESHEET
    ${doxygen-awesome-css_SOURCE_DIR}/doxygen-awesome.css)
endmacro()


function(ADD_DOXYGEN sourceDir readmeFile docsDir)
	find_package(Doxygen)
	if (NOT DOXYGEN_FOUND)
		add_custom_target(Doxygen COMMAND false
			COMMENT "Doxygen not found")
		return()
	endif()

	set(DOXYGEN_GENERATE_HTML YES)
	set(DOXYGEN_HTML_OUTPUT
		${CMAKE_CURRENT_BINARY_DIR}/${docsDir})
	set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_CURRENT_SOURCE_DIR}/${readmeFile})

	UseDoxygenAwesomeCss()
	doxygen_add_docs(Doxygen
		${CMAKE_CURRENT_SOURCE_DIR}/${sourceDir}
		${CMAKE_CURRENT_SOURCE_DIR}/${readmeFile}
		COMMENT "Generate HTML documentation"
	)
	
endfunction()