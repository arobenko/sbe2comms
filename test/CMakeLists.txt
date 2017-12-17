function (checkout_external url tag name)
    set (stamp "${CMAKE_CURRENT_BINARY_DIR}/${name}.stamp")
    if ((EXISTS ${stamp}) AND (EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${name}"))
        execute_process (
            COMMAND ${GIT_EXECUTABLE} checkout ${tag}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${name}
        )
    else ()
        execute_process (
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/${name}"
        )

        execute_process (
            COMMAND ${GIT_EXECUTABLE} clone -b ${tag} --depth 1 ${url} ${CMAKE_CURRENT_BINARY_DIR}/${name}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            RESULT_VARIABLE git_result
        )

        if (NOT "${git_result}" STREQUAL "0")
            message (WARNING "git clone/checkout of ${name} failed")
        else ()
            execute_process (
                COMMAND ${CMAKE_COMMAND} -E touch "${stamp}"
            )
        endif ()
    endif ()
endfunction ()

#################################################################

function (test_func name)
    set (schema_file "${CMAKE_CURRENT_SOURCE_DIR}/${name}/Schema.xml")
    set (mine_output_dir ${CMAKE_CURRENT_BINARY_DIR}/${name}/mine)
    add_custom_command(
        OUTPUT ${mine_output_dir}.tmp
        DEPENDS ${schema_file} ${PROJECT_NAME}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${mine_output_dir}.tmp
        COMMAND $<TARGET_FILE:${PROJECT_NAME}> -o ${mine_output_dir}.tmp -n "mine"
            --sofh-name=openFramingHeader ${schema_file}
    )

    set (orig_output_dir ${CMAKE_CURRENT_BINARY_DIR}/${name}/orig)
    set (orig_output_tgt ${name}_orig_tgt)
    add_custom_command(
        OUTPUT ${orig_output_dir}.tmp
        DEPENDS ${schema_file} ${SBE_JAR}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${orig_output_dir}.tmp
        COMMAND ${Java_JAVA_EXECUTABLE}
            -Dsbe.output.dir=${orig_output_dir}.tmp -Dsbe.target.language="cpp"
            -Dsbe.target.namespace=orig -jar ${SBE_JAR}
        ${schema_file}
    )

    set (output_tgt ${name}_output_tgt)
    add_custom_target(${output_tgt} ALL
        COMMAND ${CMAKE_COMMAND}
            -DGENERATED_MINE="${mine_output_dir}.tmp" -DOUTPUT_MINE="${mine_output_dir}"
            -DGENERATED_ORIG="${orig_output_dir}.tmp" -DOUTPUT_ORIG="${orig_output_dir}"
            -P "${CMAKE_CURRENT_LIST_DIR}/CopyGenerated.cmake"
        DEPENDS ${mine_output_dir}.tmp ${orig_output_dir}.tmp "${CMAKE_CURRENT_LIST_DIR}/CopyGenerated.cmake")

    set (tests "${CMAKE_CURRENT_SOURCE_DIR}/${name}/${name}Test.th")

    set (testName "${name}Test")

    set (runner "${name}TestRunner.cpp")

    CXXTEST_ADD_TEST (${testName} ${runner} ${tests})
    add_dependencies(${testName} ${output_tgt})
    target_include_directories (${testName} PRIVATE "${mine_output_dir}/include" "${orig_output_dir}")

    set (plugin_build_tgt ${name}_plugin_build_tgt)
    add_custom_target(${plugin_build_tgt} ALL
        COMMAND ${CMAKE_COMMAND}
            -DPROJ_DIR=${mine_output_dir}
            -DCOMMS_INSTALL_DIR="${COMMS_INSTALL_DIR}"
            -P "${CMAKE_CURRENT_LIST_DIR}/BuildPlugin.cmake"
        DEPENDS ${output_tgt} ${mine_output_dir}.tmp ${orig_output_dir}.tmp "${CMAKE_CURRENT_LIST_DIR}/BuildPlugin.cmake"
    )

endfunction ()

#################################################################

if ("${SBE_JAR}" STREQUAL "")
    message (STATUS "SBE_JAR hasn't been provided, unittesting is skipped")
    return ()
endif()

if (NOT EXISTS "${SBE_JAR}")
    message (WARNING "Specified SBE_JAR file \"${SBE_JAR}\" does NOT exist, unittesting is skipped")
    return ()
endif ()

if ("${SBE_INCLUDE_DIR}" STREQUAL "")
    get_filename_component(jar_dir ${SBE_JAR} DIRECTORY)
    set (inc_dir "${jar_dir}/../../../sbe-tool/src/main/cpp")
    set (sbe_header "${inc_dir}/sbe/sbe.h")
    if (NOT EXISTS "${sbe_header}")
        message (WARNING "Failed to identify path to to \"sbe/sbe.h\" header, please provide one using SBE_INCLUDE_DIR variable.")
        return ()
    endif ()

    set (SBE_INCLUDE_DIR "${inc_dir}")
endif ()

include_directories ("${SBE_INCLUDE_DIR}")

if ("${COMMS_INSTALL_DIR}" STREQUAL "")
    message (WARNING "Path to COMMS library installation hasn't been provided, unittesting is skipped. Please use COMMS_INSTALL_DIR variable.")
    return ()
endif ()

list (APPEND CMAKE_PREFIX_PATH "${COMMS_INSTALL_DIR}")
find_package(CommsChampion NO_MODULE)
if (NOT CC_COMMS_FOUND)
    message (WARNING "Path ${COMMS_INSTALL_DIR} doesn't contain COMMS library headers, unittesting is skipped")
    return()
endif ()
include_directories(${CC_INCLUDE_DIRS})

find_package(Java COMPONENTS Runtime)
if (NOT Java_Runtime_FOUND)
    message (WARNING "JAVA runtime hasn't been found, required for sbe-tool, unittesting is skipped.")
    return()
endif ()

find_package (Git)
if (NOT GIT_FOUND)
    message (WARNING "Git is required to check out external projects for testing")
    return()
endif ()

checkout_external("https://github.com/CxxTest/cxxtest.git" "4.4" "cxxtest")
set (CXXTEST_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/cxxtest")
set (CXXTEST_PYTHON_TESTGEN_EXECUTABLE "${CXXTEST_INCLUDE_DIR}/bin/cxxtestgen")

find_package (CxxTest)
if (NOT CXXTEST_FOUND)
    message (WARNING "Wrong cxxtest paths, must be fixed, cannot do testing...")
    return()
endif ()

include_directories ("${CXXTEST_INCLUDE_DIR}")
INCLUDE_DIRECTORIES ("${CMAKE_CURRENT_SOURCE_DIR}")

test_func (test1)
test_func (test2)
test_func (test3)
test_func (test4)
test_func (test5)
test_func (test6)
test_func (test7)
test_func (test8)
test_func (test9)
test_func (test10)
test_func (test11)
test_func (test12)
test_func (test13)
test_func (test14)
test_func (test15)
test_func (test16)
test_func (test17)
test_func (test18)
test_func (test19)
test_func (test20)
test_func (test21)
test_func (test22)
test_func (test23)
test_func (test24)
test_func (test25)
test_func (test26)
test_func (test27)
test_func (test28)
test_func (test29)
test_func (test30)

FILE(GLOB_RECURSE all_test_headers "*.h")
add_custom_target(all_test_headers_tgt SOURCES ${all_test_headers})
