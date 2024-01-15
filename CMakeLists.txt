cmake_minimum_required(VERSION 3.0)

# automagically detect if we should cross-compile
if(DEFINED ENV{TOOLCHAIN})
	set(CMAKE_C_COMPILER	$ENV{TOOLCHAIN}gcc)
	set(CMAKE_CXX_COMPILER	$ENV{TOOLCHAIN}g++)
	set(CMAKE_AR	"$ENV{TOOLCHAIN}ar" CACHE FILEPATH "CW archiver" FORCE)
endif()

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/inc/
	${CMAKE_CURRENT_LIST_DIR}/../lib/configuration/inc/
	${CMAKE_CURRENT_LIST_DIR}/../lib/lib60870/src/inc/api/
	${CMAKE_CURRENT_LIST_DIR}/../lib/lib60870/src/inc/internal/
	${CMAKE_CURRENT_LIST_DIR}/../lib/lib60870/src/hal/inc/
	${CMAKE_CURRENT_LIST_DIR}/../lib/opendnp3/cpp/lib/include/
)

add_library(lib_configuration STATIC IMPORTED)

set_target_properties(
	lib_configuration PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../lib/configuration/src/liblib_configuration.a
)

add_library(lib_iec104 STATIC IMPORTED)

set_target_properties(
	lib_iec104 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../lib/lib60870/src/liblib60870.a
)

add_library(libopendnp3 STATIC IMPORTED)

set_target_properties(
	libopendnp3 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../lib/opendnp3/cpp/lib/libopendnp3.a
)

project(Iec104MasterDnp3Master)

set(Iec104MasterDnp3Master_SRCS
   src/Iec104MasterDnp3Master.cc
   src/main.cc
   src/Dnp3ISOEHandler.cc
   src/AppConfiguration.cc
)

add_executable(Iec104MasterDnp3Master
  ${Iec104MasterDnp3Master_SRCS}
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(Iec104MasterDnp3Master PRIVATE lib_configuration lib_iec104 libopendnp3 Threads::Threads)

# target_include_directories(Iec104MasterDnp3Master PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

