# windeployqt 由 Qt6CoreTools 提供导入目标 Qt6::windeployqt
find_package(Qt6 REQUIRED COMPONENTS Core)

function(windeployqt target)

    # POST_BUILD step
    # - after build, we have a bin/lib for analyzing qt dependencies
    # - we run windeployqt on target and deploy Qt libs

    set(_windeployqt_config_arg "$<$<CONFIG:Debug>:--debug>$<$<NOT:$<CONFIG:Debug>>:--release>")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND Qt6::windeployqt
                --verbose 1
                ${_windeployqt_config_arg}
                "$<TARGET_FILE:${target}>"
        COMMENT "Deploying Qt libraries using windeployqt for compilation target '${target}' ..."
        VERBATIM
    )
endfunction()
