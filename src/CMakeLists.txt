
add_library(xcl-lib STATIC)
target_sources(xcl-lib
    PRIVATE xcl.cpp
)
target_include_directories(xcl-lib PUBLIC ${PROJECT_SOURCE_DIR}/include)
