# Copyright (c) 2020, Broadband Forum
# Copyright (c) 2020, Axiros GmbH
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from this
#    software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
cmake_minimum_required(VERSION 3.5...3.10)
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
project(OB-UDPST)
if(${CMAKE_VERSION} VERSION_GREATER "3.3.0")
        cmake_policy(SET CMP0057 NEW)
endif()
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

INCLUDE (CheckIncludeFiles)
INCLUDE (CheckFunctionExists)
INCLUDE (CheckSymbolExists)
INCLUDE (FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)

# Windows uses the statically linked vcpkg OpenSSL package to keep the GUI self-contained.
# Require crypto explicitly: silently disabling authentication would produce unusable builds.
if(WIN32)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
    find_package(OpenSSL 3.0 REQUIRED COMPONENTS Crypto)
else()
    find_package(OpenSSL 3.0)
endif()
if(OPENSSL_FOUND)
        set(HAVE_OPENSSL TRUE)
        link_directories(${OPENSSL_LIBRARY_DIRS})
        include_directories(${OPENSSL_INCLUDE_DIR})
        add_definitions(-DAUTH_KEY_ENABLE)
        set(libraries ${libraries} ${OPENSSL_LIBRARIES})
endif()

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR})

CHECK_INCLUDE_FILES (malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILES (stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES (string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES (unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES (memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILES (sys/sysinfo.h HAVE_SYSINFO_H)
CHECK_INCLUDE_FILES (sys/sysctl.h HAVE_SYSCTL_H)
CHECK_INCLUDE_FILES (linux/socket.h HAVE_SIOCGIFHWADDR)
CHECK_SYMBOL_EXISTS (LLADDR "sys/socket.h;net/if_dl.h" HAVE_NET_IF_DL_H)
CHECK_SYMBOL_EXISTS (UDP_SEGMENT "netinet/udp.h" HAVE_GSO)

CHECK_FUNCTION_EXISTS (sendmmsg HAVE_SENDMMSG)
CHECK_FUNCTION_EXISTS (recvmmsg HAVE_RECVMMSG)
CHECK_FUNCTION_EXISTS (timegm HAVE_TIMEGM)
CHECK_FUNCTION_EXISTS (mkfifo HAVE_MKFIFO)
CHECK_FUNCTION_EXISTS (getifaddrs HAVE_GETIFADDRS)
CHECK_FUNCTION_EXISTS (fork HAVE_WORKING_FORK)

OPTION(DISABLE_INT_TIMER "Disable interval timer on systems without required timer resolution (increases CPU util.)" OFF)
# The existing UDPST polling path drives timer expiry without POSIX SIGALRM.
# Use it for the Windows diagnostic build until a native timer/event loop is ready.
# This is intentionally not a claim of production-ready timing precision.
if(WIN32 AND MSVC)
    # Use the static MSVC runtime for standalone distribution.
    # This does not eliminate other third-party DLL dependencies.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
if(WIN32)
    set(DISABLE_INT_TIMER ON CACHE BOOL "Windows uses polling pending native timer support" FORCE)
endif()
OPTION(HAVE_SENDMMSG "Enable/Disable use of SendMMsg()" ON)
OPTION(HAVE_RECVMMSG "Enable/Disable use of RecvMMsg()" ON)
OPTION(HAVE_GSO "Enable/Disable use of Generic Segmentation Offload (GSO)" ON)
OPTION(RATE_LIMITING "Enable/Disable rate limiting via bandwidth management" OFF)
OPTION(AUTH_IS_OPTIONAL "Make authentication optional (considered low security and should be temporary)" OFF)
OPTION(SUPP_INVPDU_ALERT "Suppress alert when invalid control PDU is received (silently ignore)" OFF)
OPTION(SUPP_INVPDU_WARN "Suppress warning when invalid data PDU is received (silently ignore)" OFF)
OPTION(ADD_HEADER_CSUM "Add checksum to PDU headers (needed when the UDP checksum is not being utilized)" OFF)

add_definitions(-DSYSCONFDIR=\"${CMAKE_INSTALL_PREFIX}/etc\")
add_definitions(-DLOCALSTATEDIR=\"${CMAKE_INSTALL_PREFIX}/var/lib\")
add_definitions(-DLOGDIR=\"${CMAKE_INSTALL_PREFIX}/var/log\")
add_definitions(-DRUNDIR=\"${CMAKE_INSTALL_PREFIX}/run\")

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Keep the real Linux implementation intact. The Windows client still needs
# its socket and event-loop port; do not ship a false-positive udpst.exe.
if(WIN32)
    add_library(udpst_win32_srates STATIC udpst_srates.c)
    target_compile_definitions(udpst_win32_srates PRIVATE
        _WIN32_WINNT=0x0602 WIN32_LEAN_AND_MEAN)
    target_link_libraries(udpst_win32_srates PRIVATE ws2_32)
    # Compile the original control/data sources on demand to expose the next
    # MSVC portability errors without blocking the passing module build.
    add_library(udpst_win32_port_diagnostics OBJECT EXCLUDE_FROM_ALL
        udpst_control.c udpst_data.c)
    target_compile_definitions(udpst_win32_port_diagnostics PRIVATE
        _WIN32_WINNT=0x0602 WIN32_LEAN_AND_MEAN)
    # Opt-in compile probe for the actual main program; not an executable.
    add_library(udpst_win32_main_diagnostics OBJECT EXCLUDE_FROM_ALL udpst.c)
    target_compile_definitions(udpst_win32_main_diagnostics PRIVATE
        _WIN32_WINNT=0x0602 WIN32_LEAN_AND_MEAN DISABLE_INT_TIMER)
    # Link the complete native client as an opt-in target. Keep it out of
    # ALL until the Windows link and runtime checks have been validated.
    add_executable(udpst_win32_link_probe EXCLUDE_FROM_ALL
        udpst.c udpst_control.c udpst_data.c udpst_srates.c cJSON.c)
    target_compile_definitions(udpst_win32_link_probe PRIVATE
        _WIN32_WINNT=0x0602 WIN32_LEAN_AND_MEAN DISABLE_INT_TIMER)
    target_link_libraries(udpst_win32_link_probe PRIVATE ws2_32)
    if(OPENSSL_FOUND)
        target_link_libraries(udpst_win32_link_probe PRIVATE OpenSSL::Crypto)
    endif()
    set_target_properties(udpst_win32_link_probe PROPERTIES OUTPUT_NAME udpst)
    # udpst.c uses DISABLE_INT_TIMER directly in preprocessor guards.
    # config.h is generated but not included by that translation unit;
    # defining the macro on this target actually excludes POSIX SIGALRM
    # and setitimer code in MSVC diagnostics.
    message(STATUS "Windows sending-rate module build enabled; full UDPST client not yet ported")
else()
    # Define a library called udpst_core containing all core functionality.
    add_library(udpst_core udpst_control.c udpst_data.c udpst_srates.c cJSON.c)
    set(libraries udpst_core ${libraries})
    add_executable(udpst udpst.c)
    target_link_libraries(udpst ${libraries} m)
endif()

# For some reason Ninja sometimes faces a stupid error which is fixed by
# the following
if (CMAKE_GENERATOR MATCHES "Ninja")
    set(CMAKE_PLATFORM_HAS_INSTALLNAME 1)
endif()
