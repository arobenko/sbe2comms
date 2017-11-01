# GENERATED_MINE
# OUTPUT_MINE
# GENERATED_ORIG
# OUTPUT_ORIG

function (copyIfDifferent generated output)
    if (("${generated}" STREQUAL "") OR ("${output}" STREQUAL "")) 
        message (FATAL_ERROR "Bad directory name(s)")
    endif ()
    
    file(GLOB_RECURSE genFiles RELATIVE "${generated}/" "${generated}/*")
    foreach( f ${genFiles} )
      set(dest "${output}/${f}")
      set(src "${generated}/${f}")
      execute_process(
        COMMAND ${CMAKE_COMMAND}
            -E copy_if_different ${src} ${dest})
    endforeach()    
endfunction ()

copyIfDifferent ("${GENERATED_MINE}" "${OUTPUT_MINE}")
copyIfDifferent ("${GENERATED_ORIG}" "${OUTPUT_ORIG}")
