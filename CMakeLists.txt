cmake_minimum_required(VERSION 3.23)
project(Apple_Music_RPC)


set(CMAKE_CXX_STANDARD 14)
add_executable(main.cpp)
target_link_libraries(Apple_Music_RPC "${CMAKE_CURRENT_SOURCE_DIR}/lib/discord_game_sdk.dylib")
ADD_DEFINITIONS( -D_ID=1032665501018640414 )