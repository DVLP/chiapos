# Somewhere in CMakeLists.txt
message("Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")

CMAKE_MINIMUM_REQUIRED(VERSION 3.14 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Suppress strcpy error
set (_CRT_SECURE_NO_WARNINGS "1")
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.1 -mavx2")
IF(NOT CMAKE_BUILD_TYPE)
  message("WARNING SETTING DEBUG FLAG")
  message("WARNING SETTING DEBUG FLAG")
  message("WARNING SETTING DEBUG FLAG")
  message("WARNING SETTING DEBUG FLAG")
  SET(CMAKE_BUILD_TYPE "DEBUG")
ENDIF()

message("BUILDING BUILD TYPE: ${CMAKE_BUILD_TYPE}")

IF (CMAKE_BUILD_TYPE STREQUAL "DEBUG")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}") # -mavx512vl allows building without support but then causes Invalid operation
  set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
ENDIF()

set(CFLAGS "${CFLAGS}=-m64")
set(CXXFLAGS "${CXXFLAGS}=-m64")
set(LDFLAGS "${LDFLAGS}=-m64")

find_program(CCACHE_PROGRAM ccache)

if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

project(chiapos C CXX ASM)

# CMake 3.14+
include(FetchContent)
# add to cmake params to enable vcpkg
# -DCMAKE_TOOLCHAIN_FILE=path_to_vcpkg\scripts\buildsystems\vcpkg.cmake
# FetchContent_Declare(
#   mpir
#   GIT_REPOSITORY https://github.com/wbhart/mpir.git
#   GIT_TAG        mpir-3.0.0
# )
# FetchContent_MakeAvailable(mpir)

# find_package(GMP)
# if(GMP_FOUND)
#     message(STATUS "Using GMP.")
#     set(GR_MPLIB_GMP True)
#     set(MPLIB_DEFINITIONS "-DGR_MPLIB_GMP" ${GMP_DEFINITIONS})
#     set(MPLIB_INCLUDE_DIR ${GMP_INCLUDE_DIR})
#     set(MPLIB_LIBRARY ${GMP_LIBRARY})
#     set(MPLIBXX_LIBRARY ${GMPXX_LIBRARY})
#     set(MPLIB_PC_ADD_CFLAGS ${GMP_PC_ADD_CFLAGS})
#     set(MPLIB_PC_ADD_LIBS ${GMP_PC_ADD_LIBS})
# else(GMP_FOUND)
#     message(STATUS "GMP not found; this is not a problem if MPIR can be found.")
#     find_package(MPIR REQUIRED)
#     message(STATUS "MPIR found")
#     set(GR_MPLIB_MPIR True)
#     set(MPLIB_DEFINITIONS "-DGR_MPLIB_MPIR" ${MPIR_DEFINITIONS})
#     set(MPLIB_INCLUDE_DIR ${MPIR_INCLUDE_DIR})
#     set(MPLIB_LIBRARY ${MPIR_LIBRARY})
#     set(MPLIBXX_LIBRARY ${MPIRXX_LIBRARY})
#     set(MPLIB_PC_ADD_CFLAGS ${MPIR_PC_ADD_CFLAGS})
#     set(MPLIB_PC_ADD_LIBS ${MPIR_PC_ADD_LIBS})
# endif(GMP_FOUND)

# FetchContent_Declare(
#   pybind11-src
#   GIT_REPOSITORY https://github.com/pybind/pybind11.git
#   GIT_TAG        v2.6.2
# )
# FetchContent_MakeAvailable(pybind11-src)

FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG        v2.2.1
)
FetchContent_MakeAvailable(cxxopts)

# FetchContent_Declare(
#   dlfcn-win32
#   GIT_REPOSITORY https://github.com/dlfcn-win32/dlfcn-win32.git
#   GIT_TAG        v1.3.0
# )
# FetchContent_MakeAvailable(dlfcn-win32)
# find_package(dlfcn-win32 REQUIRED)
# set(CMAKE_DL_LIBS dlfcn-win32::dl)

# FetchContent_Declare(
#   gulrak
#   GIT_REPOSITORY https://github.com/gulrak/filesystem.git
#   GIT_TAG        v1.5.6
# )
# FetchContent_MakeAvailable(gulrak)

set(FSE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/FiniteStateEntropy/lib)
set(FSE_FILES
    ${FSE_LIB}/fse_compress.c
    ${FSE_LIB}/fse_decompress.c
    ${FSE_LIB}/entropy_common.c
    ${FSE_LIB}/hist.c
)

include_directories(
  ${INCLUDE_DIRECTORIES}
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/include
  ${cxxopts_SOURCE_DIR}/include
  ${gulrak_SOURCE_DIR}/include/ghc
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/FiniteStateEntropy/lib
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/test
  ${CMAKE_CURRENT_SOURCE_DIR}/plotter
  # ${CMAKE_CURRENT_SOURCE_DIR}/lib/bls-signatures/build/src/Debug
  # ${CMAKE_CURRENT_SOURCE_DIR}/lib/bls-signatures/build
  # ${CMAKE_CURRENT_SOURCE_DIR}/lib/bls-signatures/build/_deps/relic-build/include
  # ${CMAKE_CURRENT_SOURCE_DIR}/lib/bls-signatures/build/_deps/relic-src/include
  # ${CMAKE_CURRENT_SOURCE_DIR}/lib/relic_ietf_64/include  # remember to change WSIZE to 32 or MSVC won't build - this is only to generate initial id so performance is irrelevant
  )

add_library(fse STATIC ${FSE_FILES})

IF (MSVC)
  # increase stack size for large bitsets - 1GB stack YOLO
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:\"1073741824\" /LARGEADDRESSAWARE")

  IF (CMAKE_BUILD_TYPE STREQUAL "RELEASE")
    # MSVC
    # set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /Oy /Ob3 /LTCG /GL /arch:AVX2 /fp:fast /Gv /F 1073741824")
    # set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /O2 /Oy /Ob3 /LTCG /GL /arch:AVX2 /fp:fast /Gv")
    # MSVC/Clang
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /Oy /Ob3 /GL /arch:AVX2 /fp:fast /Gv")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /O2 /Oy /Ob3 /GL /arch:AVX2 /fp:fast /Gv")
  ELSE()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Ob1 /Zi")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Ob1 /Zi")
  ENDIF()

  # set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
  # set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zi")
ELSE()
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--stack,1073741824")

  IF (CMAKE_BUILD_TYPE STREQUAL "RELEASE")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Ofast -flto -ffast-math -mfpmath=sse -mpc64 -mno-push-args -march=native -pipe -fomit-frame-pointer") # -fprofile-generate=/optprofile -fbranch-probabilities -fprofile-values -funroll-loops -fpeel-loops -ftracer -fvpt -finline-functions -fipa-cp -fipa-cp-clone -fipa-bit-cp -fpredictive-commoning -fsplit-loops -funswitch-loops -fgcse-after-reload -ftree-loop-vectorize -ftree-slp-vectorize -fvect-cost-model=dynamic -ftree-loop-distribute-patterns -fprofile-correction")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Ofast -flto -ffast-math -mfpmath=sse -mpc64 -mno-push-args -march=native -pipe -fomit-frame-pointer") # -fprofile-generate=/optprofile -fbranch-probabilities -fprofile-values -funroll-loops -fpeel-loops -ftracer -fvpt -finline-functions -fipa-cp -fipa-cp-clone -fipa-bit-cp -fpredictive-commoning -fsplit-loops -funswitch-loops -fgcse-after-reload -ftree-loop-vectorize -ftree-slp-vectorize -fvect-cost-model=dynamic -ftree-loop-distribute-patterns -fprofile-correction")
  ELSE()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -flto -Wall -g -march=native -pipe -fomit-frame-pointer -fprofile-generate=/optprofile")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -flto -Wall -g -march=native -pipe -fomit-frame-pointer -fprofile-generate=/optprofile")
    
  ENDIF()

  # set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")
  # set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g")

ENDIF()

IF (CMAKE_BUILD_TYPE STREQUAL "ASAN")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")
set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")
ENDIF()

IF (CMAKE_BUILD_TYPE STREQUAL "TSAN")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=thread")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -fsanitize=thread")
set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=thread")
ENDIF()

IF (APPLE)
# on macOS "uname -m" returns the architecture (x86_64 or arm64)
execute_process(
    COMMAND uname -m
    RESULT_VARIABLE result
    OUTPUT_VARIABLE OSX_NATIVE_ARCHITECTURE
    OUTPUT_STRIP_TRAILING_WHITESPACE)
ENDIF()

IF (WIN32)
set(BLAKE3_SRC
    src/b3/blake3.c
    src/b3/blake3_portable.c
    src/b3/blake3_dispatch.c
    src/b3/blake3_avx2.c
    # src/b3/blake3_avx512.c
    src/b3/blake3_sse41.c
)
ELSEIF(OSX_NATIVE_ARCHITECTURE STREQUAL "arm64")
set(BLAKE3_SRC
    src/b3/blake3.c
    src/b3/blake3_portable.c
    src/b3/blake3_dispatch.c
)
ELSE()
set(BLAKE3_SRC
    src/b3/blake3.c
    src/b3/blake3_portable.c
    src/b3/blake3_dispatch.c
    src/b3/blake3_avx2_x86-64_unix.S
    # src/b3/blake3_avx512_x86-64_unix.S
    src/b3/blake3_sse41_x86-64_unix.S
)
ENDIF()

#pybind11_add_module(chiapos ${CMAKE_CURRENT_SOURCE_DIR}/python-bindings/chiapos.cpp src/chacha8.c ${BLAKE3_SRC})

# add_executable(ProofOfSpace
#     src/cli.cpp
#     src/chacha8.c
#     ${BLAKE3_SRC}
# )

# IdgenLib - commented out here as being used as DLL
# add_library( IdgenLib SHARED IMPORTED )
# set_target_properties( IdgenLib PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/bls-signatures/build/src/Debug/IdgenLib.dll )
# set_target_properties( IdgenLib PROPERTIES IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/bls-signatures/build/src/Debug/IdgenLib.dll )
# link_directories(lib/bls-signatures/build)
add_executable(Plotter
    plotter/plotter.cpp
    src/chacha8.c
    ${BLAKE3_SRC}
)

# Pasted for now but fix RELEASE - DEBUG flag!!!
target_compile_options(Plotter PRIVATE -O3 -Ofast -flto -ffast-math -mfpmath=sse -mpc64 -mno-push-args -march=native -pipe -fomit-frame-pointer) # -fprofile-generate=/optprofile -fbranch-probabilities -fprofile-values -funroll-loops -fpeel-loops -ftracer -fvpt -finline-functions -fipa-cp -fipa-cp-clone -fipa-bit-cp -fpredictive-commoning -fsplit-loops -funswitch-loops -fgcse-after-reload -ftree-loop-vectorize -ftree-slp-vectorize -fvect-cost-model=dynamic -ftree-loop-distribute-patterns -fprofile-correction)
# target_link_libraries(Plotter IdgenLib)

# add_executable(RunTests
#     tests/test-main.cpp
#     tests/test.cpp
#     src/chacha8.c
#     ${BLAKE3_SRC}
# )

find_package(Threads REQUIRED)

add_library(uint128 STATIC uint128_t/uint128_t.cpp)
target_include_directories(uint128 PUBLIC uint128_t)

target_compile_features(fse PUBLIC cxx_std_17)
# target_compile_features(chiapos PUBLIC cxx_std_17)
# target_compile_features(RunTests PUBLIC cxx_std_17)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # target_link_libraries(chiapos PRIVATE fse Threads::Threads)
  # target_link_libraries(ProofOfSpace fse Threads::Threads)
  target_link_libraries(Plotter fse Threads::Threads)
  # target_link_libraries(RunTests fse Threads::Threads)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
  # target_link_libraries(chiapos PRIVATE fse Threads::Threads)
  # target_link_libraries(ProofOfSpace fse Threads::Threads)
  target_link_libraries(Plotter fse Threads::Threads)
  # target_link_libraries(RunTests fse Threads::Threads)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  # target_link_libraries(chiapos PRIVATE fse Threads::Threads)
  # target_link_libraries(ProofOfSpace fse Threads::Threads)
  target_link_libraries(Plotter fse Threads::Threads)
  # target_link_libraries(RunTests fse Threads::Threads)
elseif (MSVC)
  # target_link_libraries(chiapos PRIVATE fse Threads::Threads uint128 ${dlfcn-win32_LIBRARIES})
  # target_link_libraries(ProofOfSpace fse Threads::Threads uint128 ${dlfcn-win32_LIBRARIES})
  target_link_libraries(Plotter fse Threads::Threads uint128)
  # target_link_libraries(RunTests fse Threads::Threads uint128)
elseif (CXX)
  # target_link_libraries(chiapos PRIVATE fse Threads::Threads uint128 ${dlfcn-win32_LIBRARIES})
  # target_link_libraries(ProofOfSpace fse Threads::Threads uint128 ${dlfcn-win32_LIBRARIES})
  target_link_libraries(Plotter fse Threads::Threads uint128)
  # target_link_libraries(RunTests fse Threads::Threads uint128)
else()
  # target_link_libraries(chiapos PRIVATE fse Threads::Threads uint128 ${dlfcn-win32_LIBRARIES})
  # target_link_libraries(ProofOfSpace fse Threads::Threads uint128 ${dlfcn-win32_LIBRARIES})
  target_link_libraries(Plotter fse Threads::Threads uint128)
  # target_link_libraries(RunTests fse Threads::Threads uint128)
  # TODO: why stdc++fs needed?
  # target_link_libraries(chiapos PRIVATE fse stdc++fs Threads::Threads)
  # target_link_libraries(ProofOfSpace fse stdc++fs Threads::Threads)
  # target_link_libraries(Plotter fse stdc++fs idgen Threads::Threads)
  # target_link_libraries(RunTests fse stdc++fs Threads::Threads)
endif()

# enable_testing()
# add_test(NAME RunTests COMMAND RunTests)
