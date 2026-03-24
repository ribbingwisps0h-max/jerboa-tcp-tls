cmake_minimum_required(VERSION 3.28)
project(jerboa-tcp-tls VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Find dependencies ──────────────────────────────────────────────────────────
find_package(OpenSSL 3.0 REQUIRED)
find_package(Boost 1.83 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

# ── Sources ────────────────────────────────────────────────────────────────────
set(SOURCES
        src/main.cpp
        src/tls_server.cpp
        src/session.cpp
)

# ВАЖНО: Сначала создаем таргет
add_executable(${PROJECT_NAME} ${SOURCES})

# ── Compiler & Linker flags ────────────────────────────────────────────────────
target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
        $<$<CONFIG:Debug>:-g -fsanitize=address,undefined -fno-omit-frame-pointer>
)

target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME} PRIVATE
        OpenSSL::SSL OpenSSL::Crypto Boost::system Threads::Threads
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT=1
        BOOST_ALLOW_DEPRECATED_HEADERS
)