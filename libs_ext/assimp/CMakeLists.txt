cmake_minimum_required(VERSION 3.5)

set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "  " FORCE) # We do not have zlib so let assimp build it own.
set(ASSIMP_NO_EXPORT ON CACHE BOOL "  " FORCE) # We won't be exporting from Assimp.

set(BUILD_SHARED_LIBS OFF CACHE BOOL "  " FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "  " FORCE) # We do not need the tools.
set(ASSIMP_BUILD_ASSIMP_VIEW OFF CACHE BOOL "  " FORCE) # We do not need the tools.
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "  " FORCE) # We've deleted the tests dir as the test files are huge.
set(ASSIMP_INSTALL OFF CACHE BOOL "  " FORCE)
set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "  " FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "  " FORCE)

add_subdirectory(assimp) # The version of the code was 5.1.5 official release