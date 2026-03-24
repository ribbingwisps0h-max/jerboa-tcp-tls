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

# ── Compiler flags ─────────────────────────────────────────────────────────────
# Указываем флаги для Debug сборки
# Для Debug: добавляем санитайзеры и отладочную инфу
target_compile_options(jerboa-tcp-tls PRIVATE
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-fsanitize=address,undefined>
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
)

# Для Release: добавляем оптимизацию
target_compile_options(jerboa-tcp-tls PRIVATE
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-DNDEBUG>
)

# Важно: для санитайзеров нужна и линковка!
target_link_options(jerboa-tcp-tls PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

# ── Find dependencies ──────────────────────────────────────────────────────────
find_package(OpenSSL 3.0 REQUIRED)

# Try to find Boost.Asio (header-only component, but we need Boost.System)
find_package(Boost 1.83.0 REQUIRED COMPONENTS system)

# Threads
find_package(Threads REQUIRED)

# ── Sources ────────────────────────────────────────────────────────────────────
set(SOURCES
        src/main.cpp
        src/tls_server.cpp
        src/session.cpp
        src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES}
        src/main.cpp)

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

# ── Feature-test macros required for C++23 print / expected ───────────────────
target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT=1
        # Suppress Boost deprecation noise on newer standards
        BOOST_ALLOW_DEPRECATED_HEADERS
)

# ── Install ────────────────────────────────────────────────────────────────────
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

message(STATUS "────────────────────────────────────────")
message(STATUS "  jerboa-tcp-tls ${PROJECT_VERSION}")
message(STATUS "  Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ std    : ${CMAKE_CXX_STANDARD}")
message(STATUS "  OpenSSL    : ${OPENSSL_VERSION}")
message(STATUS "  Boost      : ${Boost_VERSION}")
message(STATUS "────────────────────────────────────────")

