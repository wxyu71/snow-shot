include_guard(GLOBAL)

# Keep test selection in a CMake module so both the production workspace
# and isolated CTest fixtures exercise the same executable policy.
#
# Interactive, end-to-end and benchmark tests require explicit opt-in. They
# must not unexpectedly run in the default deterministic test target.
function(snow_add_deterministic_test_runner)
    if(TARGET snow-tests)
        return()
    endif()
    if(NOT TARGET snow-all)
        message(FATAL_ERROR "snow-tests requires the workspace snow-all target")
    endif()

    add_custom_target(snow-tests
        COMMAND "${CMAKE_CTEST_COMMAND}"
            --test-dir "${CMAKE_BINARY_DIR}"
            -C "$<CONFIG>"
            -LE "interactive|e2e|benchmark"
            --output-on-failure
        USES_TERMINAL
        VERBATIM
    )
    add_dependencies(snow-tests snow-all)
endfunction()
