# The upstream NSIS fixture is not the Community production installer.
# Keep it available to NSIS developers and the full Windows unit suite.
# The mandatory installer-contracts job and production smoke test exercise Inno 7.
function(snow_shot_add_nsis_running_app_test powershell script)
    add_test(NAME snow-shot-installer-running-app-tests
        COMMAND "${powershell}" -NoProfile -ExecutionPolicy Bypass -File "${script}")
    set_tests_properties(snow-shot-installer-running-app-tests PROPERTIES
        LABELS "unit;windows;installer-nsis" TIMEOUT 180)
endfunction()
