cmake_minimum_required(VERSION 3.30.0)

project(blink VERSION 0.1.0 LANGUAGES ASM C)

set(SOURCES
    src/startup_stm32l011xx.s
    src/tx_initialize_low_level.S
    src/main.c)

add_executable(${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PUBLIC STM32L011xx)
target_sources(${PROJECT_NAME} PUBLIC ${SOURCES})

if(NOT CMAKE_CURRENT_BINARY_DIR MATCHES "stm32")
    include(CTest)
    enable_testing()
endif()

if(CMAKE_CURRENT_BINARY_DIR MATCHES "stm32")
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}.elf")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${SIZE} $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "Running size on the built binary"
    )

    set(OUTPUT_BINARY ${CMAKE_CURRENT_BINARY_DIR}/bin/${PROJECT_NAME}.bin)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${OUTPUT_BINARY}
        COMMENT "Convert elf to bin"
    )

    set(FLASHER /Applications/STMicroelectronics/STM32Cube/STM32CubeProgrammer/STM32CubeProgrammer.app/Contents/MacOs/bin/STM32_Programmer_CLI)
    set(START_ADDRESS 0x08000000)

    add_custom_target(flash
        COMMAND ${FLASHER} -c port=SWD -w ${OUTPUT_BINARY} ${START_ADDRESS} -s ${START_ADDRESS}
        COMMENT "Flashing binary file using STM32CubeProgrammer CLI"
        DEPENDS ${OUTPUT_BINARY}
    )

    add_subdirectory(threadx)

    target_compile_definitions(threadx PUBLIC TX_DISABLE_ERROR_CHECKING)
    target_compile_definitions(threadx PUBLIC TX_DISABLE_PREEMPTION_THRESHOLD)
    target_compile_definitions(threadx PUBLIC TX_DISABLE_NOTIFY_CALLBACKS)
    target_compile_definitions(threadx PUBLIC TX_DISABLE_REDUNDANT_CLEARING)
    target_compile_definitions(threadx PUBLIC TX_DISABLE_STACK_FILLING)
    target_compile_definitions(threadx PUBLIC TX_NOT_INTERRUPTABLE)
    target_compile_definitions(threadx PUBLIC TX_TIMER_PROCESS_IN_ISR)
    
    target_link_libraries(${PROJECT_NAME} azrtos::threadx)
endif()