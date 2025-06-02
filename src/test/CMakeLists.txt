# option
add_compile_options(-Wall -Wno-unknown-pragmas)

# include dir
include_directories(/usr/local/include/)
include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${PROJECT_SOURCE_DIR}/dependencies/json/single_include/)
include_directories(
  ${PROJECT_SOURCE_DIR}/dependencies/airouting2.0/algorithm/algorithm-core/src/)

# link dir
link_directories(/usr/local/lib/)
link_directories(
  ${PROJECT_SOURCE_DIR}/dependencies/airouting2.0/algorithm/algorithm-core/lib/)

# source
set(source_dir ${PROJECT_SOURCE_DIR}/src/)
file(GLOB_RECURSE source ${source_dir}*.cc)

# link lib
set(link_libraries
    glog
    protobuf
    ortools
    aws-cpp-sdk-s3
    aws-cpp-sdk-core
    algo_fw
    httpserver
    restclient-cpp
    redis
    server
    airv2.so
)

# exe output
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/${EXECUTABLE_OUTPUT_PATH})
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

# add exe
set(executable_file test)
add_executable(${executable_file} ${source})
target_link_libraries(${executable_file} ${link_libraries})
