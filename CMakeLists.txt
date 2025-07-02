cmake_minimum_required(VERSION 3.20)

project(fuzzoff DESCRIPTION "A unikernel for eBPF programs.")

enable_language(ASM)

set(FS_TEST_DRIVER_SRCS
	./driver/fs_test_main.c
)

set(DRIVER_SRCS
	./driver/simple_test_main.c
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
	./src/virtio_driver.c
	./src/blk.c
	${FS_TEST_DRIVER_SRCS}
)


if(NOT DEBUG_LEVEL)
	set(DEBUG_LEVEL 1)
endif()

add_executable(fuzzoff ${SRCS})
target_compile_options(fuzzoff PRIVATE -mcmodel=large)
target_include_directories(fuzzoff PRIVATE include)
set(ASM_OPTIONS "-x assembler-with-cpp")
target_link_options(fuzzoff PRIVATE -static -T${CMAKE_SOURCE_DIR}/ld/standard.lds -mcmodel=large -mno-relax)
configure_file(${CMAKE_SOURCE_DIR}/cmake/build_config.h ${CMAKE_SOURCE_DIR}/include )


add_executable(standalone-driver ${DRIVER_SRCS})
target_compile_options(standalone-driver PRIVATE -mcmodel=large)
target_include_directories(standalone-driver PRIVATE include)
target_link_options(standalone-driver PRIVATE -static)


# Targets for manipulating the testing environment.

# Create a simple qcow2 image for testing:
add_custom_target(make-simple-qcow2 qemu-img "create" "-f" "qcow2" "${CMAKE_SOURCE_DIR}/test/fs/simple.qcow2" "512B")
