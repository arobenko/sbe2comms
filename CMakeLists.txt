cmake_minimum_required (VERSION 3.1)
project ("sbe2comms")

option (SBE2COMMS_NO_WARN_AS_ERR "Do NOT treat warning as error" OFF)

if (NOT CMAKE_CXX_STANDARD)
    set (CMAKE_CXX_STANDARD 11)
endif()

if ((CMAKE_COMPILER_IS_GNUCC) OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
    set (extra_flags_list
        "-Wall" "-Wextra" "-Wcast-align" "-Wcast-qual" "-Wctor-dtor-privacy"
        "-Wmissing-include-dirs"
        "-Woverloaded-virtual" "-Wredundant-decls" "-Wshadow" "-Wundef" "-Wunused"
        "-Wno-unknown-pragmas" "-fdiagnostics-show-option"
    )

    if (CMAKE_COMPILER_IS_GNUCC)
        list (APPEND extra_flags_list
            "-Wnoexcept" "-Wlogical-op" "-Wstrict-null-sentinel"
        )

        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8")
            list (APPEND extra_flags_list
                "-Wno-pragmas" "-Wno-type-limits"
            )
        endif()

    endif ()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        list (APPEND extra_flags_list "-Wno-dangling-field -Wno-unused-command-line-argument")
    endif ()
    
    if (NOT SBE2COMMS_NO_WARN_AS_ERR)
        list (APPEND extra_flags_list "-Werror")
    endif ()
    
    string(REPLACE ";" " " extra_flags "${extra_flags_list}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${extra_flags}")
    
elseif (MSVC)

    add_definitions( "/wd4503" "/wd4309" "/wd4267" "-D_SCL_SECURE_NO_WARNINGS")

    if (NOT SBE2COMMS_NO_WARN_AS_ERR)
        add_definitions("/WX")
    endif ()
    
endif ()

find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})
add_definitions(${LIBXML2_DEFINITIONS})

find_package(Boost REQUIRED
    COMPONENTS program_options filesystem)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

set (
    src
    "main.cpp"
    "xml_wrap.cpp"
    "MessageSchema.cpp"
    "Type.cpp"
    "BasicType.cpp"
    "CompositeType.cpp"
    "EnumType.cpp"
    "SetType.cpp"
    "Message.cpp"
    "Field.cpp"
    "BasicField.cpp"
    "GroupField.cpp"
    "DataField.cpp"
    "DB.cpp"
    "BuiltIn.cpp"
    "get.cpp"
    "prop.cpp"
    "output.cpp"
)

add_executable(${PROJECT_NAME} ${src})
target_link_libraries(${PROJECT_NAME} ${LIBXML2_LIBRARIES} ${Boost_LIBRARIES})


FILE(GLOB_RECURSE all_headers "*.h")
add_custom_target(all_headers_tgt SOURCES ${all_headers})
