cmake_minimum_required(VERSION 3.20)

project(unibpform DESCRIPTION "A unikernel for eBPF programs.")

enable_language(ASM)

set(DRIVER_SRCS
	./driver/main.c
)
set(SRCS 
	./src/boot.S
	./src/io.c
	./src/smemory.c
	./src/strap.S
	./src/strap_handler.c
	./src/system.S
	./src/system.c
	./src/pci.c
	./src/util.c
	./src/virtio.c
	./src/blk.c
	${DRIVER_SRCS}
)


if(NOT DEBUG_LEVEL)
	set(DEBUG_LEVEL 1)
endif()

add_executable(unibpform ${SRCS})
target_compile_options(unibpform PRIVATE -mcmodel=large)
target_include_directories(unibpform PRIVATE include)
set(ASM_OPTIONS "-x assembler-with-cpp")
target_link_options(unibpform PRIVATE -static -T${CMAKE_SOURCE_DIR}/ld/standard.lds -mcmodel=large -mno-relax)
#file(GENERATE OUTPUT ${CMAKE_SOURCE_DIR}/include INPUT ${CMAKE_SOURCE_DIR}/cmake/build_config.h)
configure_file(${CMAKE_SOURCE_DIR}/cmake/build_config.h ${CMAKE_SOURCE_DIR}/include )
#add_dependencies(unibpform main_lib)


add_executable(standalone-driver ${DRIVER_SRCS})
target_compile_options(standalone-driver PRIVATE -mcmodel=large)
target_include_directories(standalone-driver PRIVATE include)
target_link_options(standalone-driver PRIVATE -static)



