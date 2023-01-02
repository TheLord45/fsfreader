set(CMAKE_CXX_STANDARD 17)

add_executable(fsfreader fsfreader.cpp
               readtp4.cpp
               expand.cpp)

add_definitions(-D_GNU_SOURCE)

if (DEBUG)
   add_definitions(-g)
endif(DEBUG)

add_definitions(-pedantic -fexceptions -Wextra -std=c++17)

target_link_libraries(fsfreader m ${LIBS})

install(TARGETS fsfreader RUNTIME DESTINATION usr)
