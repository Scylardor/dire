function(FIND_RAPIDJSON_DEPENDENCY)
	find_package(RapidJSON CONFIG REQUIRED)
	if(NOT TARGET RapidJSON::RapidJSON)
		add_library(RapidJSON::RapidJSON INTERFACE IMPORTED)
    	set_target_properties(RapidJSON::RapidJSON PROPERTIES
        	INTERFACE_INCLUDE_DIRECTORIES "${RapidJSON_INCLUDE_DIRS}")
	endif()
endfunction()