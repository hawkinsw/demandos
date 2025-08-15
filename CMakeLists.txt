cmake_minimum_required(VERSION 3.20)

project(demandos DESCRIPTION "A unikernel for eBPF programs.")

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
	./src/strap.S
	./src/strap_handler.c
	./src/system.c
	./src/pci.c
	./src/util.c
	./src/virtio.c
	./src/virtio_driver.c
	./src/blk.c
	./src/e.c
	./src/os.c
	./src/argcv.c
	./src/memory.c
	./src/ext2.c
	./src/demandos.c
)

set(RUNTIME_SRCS 
	./src/runtime.c
)

set(TEST_SRCS 
	./test/unit_test.c
)

set(FORMATTABLE_FILES "${SRCS}")
list(APPEND FORMATTABLE_FILES "${RUNTIME_SRCS}")
list(APPEND FORMATTABLE_FILES "${TEST_SRCS}")
list(FILTER FORMATTABLE_FILES EXCLUDE REGEX ".*.S$")
list(TRANSFORM FORMATTABLE_FILES PREPEND "${CMAKE_SOURCE_DIR}/" OUTPUT_VARIABLE FORMATTABLE_FILES)
list(JOIN FORMATTABLE_FILES "\n" FORMATTABLE_FILES)

if(NOT DEBUG_LEVEL)
	set(DEBUG_LEVEL 1)
endif()

if(ENABLE_TESTS)
	set(ENABLE_TESTS 1)
else()
	set(ENABLE_TESTS 0)
endif()

add_executable(demandos ${SRCS} ${RUNTIME_SRCS})
target_compile_options(demandos PRIVATE -mcmodel=large)
target_include_directories(demandos PRIVATE include)
set(ASM_OPTIONS "-x assembler-with-cpp")
target_link_options(demandos PRIVATE -static -T${CMAKE_SOURCE_DIR}/ld/standard.lds -mcmodel=large -mno-relax)
configure_file(${CMAKE_SOURCE_DIR}/cmake/build_config.h ${CMAKE_SOURCE_DIR}/include )

add_executable(demandos-test ${SRCS} ${TEST_SRCS})
target_compile_options(demandos-test PRIVATE -mcmodel=large)
target_include_directories(demandos-test PRIVATE include)
set(ASM_OPTIONS "-x assembler-with-cpp")
target_link_options(demandos-test PRIVATE -static -T${CMAKE_SOURCE_DIR}/ld/standard.lds -mcmodel=large -mno-relax)

add_executable(standalone-driver ${DRIVER_SRCS})
target_compile_options(standalone-driver PRIVATE -mcmodel=large)
target_include_directories(standalone-driver PRIVATE include)
target_link_options(standalone-driver PRIVATE -static)

# Targets for manipulating the testing environment.

# Create a simple qcow2 image for testing:
add_custom_target(make-simple-qcow2 qemu-img "create" "-f" "qcow2" "${CMAKE_SOURCE_DIR}/test/fs/simple.qcow2" "512B")

file(GENERATE OUTPUT ${CMAKE_SOURCE_DIR}/cicd/clang-format-configure CONTENT "${FORMATTABLE_FILES}")
add_custom_target(clang-format COMMAND ${CMAKE_SOURCE_DIR}/cicd/format.sh "format" "${CMAKE_SOURCE_DIR}/cicd/clang-format-configure")

