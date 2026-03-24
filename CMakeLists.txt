cmake_minimum_required(VERSION 3.28)
project(jerboa-tcp-tls VERSION 1.0.0 LANGUAGES CXX)

# ── C++23 ─────────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Build type defaults ────────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ── Find dependencies ──────────────────────────────────────────────────────────
find_package(OpenSSL 3.0 REQUIRED)
find_package(Boost 1.83.0 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

# ── Sources ────────────────────────────────────────────────────────────────────
# Убрал дубликаты main.cpp
set(SOURCES
        src/main.cpp
        src/tls_server.cpp
        src/session.cpp
)

# Сначала СОЗДАЕМ исполняемый файл
add_executable(${PROJECT_NAME} ${SOURCES})

# ── Compiler flags ─────────────────────────────────────────────────────────────
# Теперь, когда проект создан, можно вешать на него флаги
target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
        -g
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        >
        $<$<CONFIG:Release>:
        -O3
        -DNDEBUG
        >
)

target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
        -fsanitize=address,undefined
        >
)

# ── Include dirs ───────────────────────────────────────────────────────────────
target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${Boost_INCLUDE_DIRS}
)

# ── Link libraries ─────────────────────────────────────────────────────────────
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        Boost::system
        Threads::Threads
)

# ── Feature-test macros ────────────────────────────────────────────────────────
target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT=1
        BOOST_ALLOW_DEPRECATED_HEADERS
)

# ── Install ────────────────────────────────────────────────────────────────────
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# ── Status ─────────────────────────────────────────────────────────────────────
message(STATUS "────────────────────────────────────────")
message(STATUS "  Project    : ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ std    : ${CMAKE_CXX_STANDARD}")
message(STATUS "  OpenSSL    : ${OPENSSL_VERSION}")
message(STATUS "  Boost      : ${Boost_VERSION}")
message(STATUS "────────────────────────────────────────")