# Locate GTest
find_package(GTest REQUIRED)

add_executable( run_tests run_tests.cpp)

target_compile_features(run_tests PRIVATE cxx_std_17)

target_include_directories( run_tests
	PRIVATE		
		${PROJECT_SOURCE_DIR}/include/
		${Boost_INCLUDE_DIRS}		
		${GTEST_INCLUDE_DIRS}
)

target_link_libraries( run_tests 
	PRIVATE    	
		dl     
		Boost::timer   
	    pugixml
	    ${GTEST_LIBRARIES}
	    pthread
		rt
)
