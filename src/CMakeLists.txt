cmake_minimum_required(VERSION 3.16)
project(qoin_server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)

add_executable(qoin_server
	server_main.cpp
	Trader.cpp
	Api.cpp
	Socket.cpp
	BSocket.cpp
)

target_link_libraries(qoin_server PRIVATE Drogon::Drogon ssl crypto)
