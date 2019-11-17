# Basic example

add_executable( basic basic.cpp)

target_compile_features(basic PRIVATE cxx_std_17)

target_include_directories( basic
	PRIVATE		
		${PROJECT_SOURCE_DIR}/include/
		${Boost_INCLUDE_DIRS}		
)

target_link_libraries( basic 
	PRIVATE    	
		dl     
		Boost::timer   
	    pugixml
		rt
)
