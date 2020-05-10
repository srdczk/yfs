cmake_minimum_required(VERSION 3.13)
project(rpc)

set(CMAKE_CXX_STANDARD 17)

add_executable(rpc
        main.cpp
        Poller.cpp
        Poller.h
        SyncQueue.h
        ThreadPool.h
        ThreadPool.cpp
        Connection.h
        Connection.cpp
        )

target_link_libraries(rpc pthread)