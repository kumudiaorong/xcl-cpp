add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PRIVATE xcl.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)