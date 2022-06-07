file(GLOB SRCS *.cpp)

find_package(Threads)

add_library(
    eloop
    ${SRCS}
)

target_link_libraries(
    eloop
    pthread
)
