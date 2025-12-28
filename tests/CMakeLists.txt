add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain lab1_core)
target_include_directories(tests PRIVATE ${PROJECT_SOURCE_DIR}/src)
