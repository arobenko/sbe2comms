set (INTERNAL_LIBXML_TGT)
while (TRUE)
    if (UNIX)
        # Use libxml2 from system repositories
        break()
    endif ()

    if ((NOT "${LIBXML2_INCLUDE_DIR}" STREQUAL "") AND (NOT "${}" STREQUAL ""))
        # External build of libxml2 is provided
        break()
    endif ()

    if (NOT MSVC)
        message (FATAL_ERROR "At this moment only MSVC compiler is supported for windows builds")
    endif ()

    set (INTERNAL_LIBXML_TGT "libxml2_tgt")
    set (LIBXML2_DIR "${CMAKE_CURRENT_BINARY_DIR}/libxml2")
    set (LIBXML2_SRC_DIR "${LIBXML2_DIR}/src")
    set (LIBXML2_BIN_DIR "${LIBXML2_SRC_DIR}/win32")

    set (LIBXML2_CRUNTIME)
    if (("${CMAKE_BUILD_TYPE}" STREQUAL "") OR ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"))
        set (LIBXML2_CRUNTIME "cruntime=/MDd")
    endif ()

    include(ExternalProject)
    ExternalProject_Add(
        ${INTERNAL_LIBXML_TGT}
        PREFIX "${LIBXML2_DIR}"
        STAMP_DIR "${LIBXML2_DIR}"
        GIT_REPOSITORY "git://git.gnome.org/libxml2"
        GIT_TAG "v2.9.7"
        CONFIGURE_COMMAND
            cscript ${LIBXML2_BIN_DIR}/configure.js ftp=no html=no iconv=no compiler=msvc static=yes ${LIBXML2_CRUNTIME} bindir=install\\bin incdir=install\\include libdir=install\\lib sodir=install\\bin
        SOURCE_DIR "${LIBXML2_SRC_DIR}"
        BINARY_DIR "${LIBXML2_BIN_DIR}"
        BUILD_COMMAND
            nmake /f Makefile.msvc
        INSTALL_COMMAND
            nmake /f Makefile.msvc install
    )

    set (LIBXML2_FOUND TRUE)
    set (LIBXML2_INCLUDE_DIR "${LIBXML2_BIN_DIR}/install/include/libxml2")
    set (LIBXML2_LIBRARIES "${LIBXML2_BIN_DIR}/install/lib/libxml2_a.lib")
    set (LIBXML2_DEFINITIONS "/DLIBXML_STATIC")
    break()
endwhile ()

if (NOT LIBXML2_FOUND)
    find_package(LibXml2 REQUIRED)
endif ()
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
    "DB.cpp"
    "ProgramOptions.cpp"
    "MessageSchema.cpp"
    "Type.cpp"
    "BasicType.cpp"
    "CompositeType.cpp"
    "EnumType.cpp"
    "SetType.cpp"
    "RefType.cpp"
    "Message.cpp"
    "Field.cpp"
    "BasicField.cpp"
    "GroupField.cpp"
    "DataField.cpp"
    "BuiltIn.cpp"
    "MsgId.cpp"
    "MsgInterface.cpp"
    "AllMessages.cpp"
    "AllFields.cpp"
    "MessageHeaderLayer.cpp"
    "OpenFramingHeaderLayer.cpp"
    "TransportFrame.cpp"
    "FieldBase.cpp"
    "common.cpp"
    "prop.cpp"
    "output.cpp"
    "log.cpp"
    "Cmake.cpp"
    "TransportMessage.cpp"
    "Protocol.cpp"
    "Plugin.cpp"
    "Doxygen.cpp"
)

add_executable(${PROJECT_NAME} ${src})
target_link_libraries(${PROJECT_NAME} ${LIBXML2_LIBRARIES} ${Boost_LIBRARIES})

if (WIN32)
    target_link_libraries(${PROJECT_NAME} Setupapi.lib Ws2_32.lib imm32.lib winmm.lib)
endif ()

if (NOT "${INTERNAL_LIBXML_TGT}" STREQUAL "")
    add_dependencies(${PROJECT_NAME} ${INTERNAL_LIBXML_TGT})
endif ()

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${SBE2COMMS_INSTALL_DIR}/bin
)

FILE(GLOB_RECURSE all_headers "*.h")
add_custom_target(all_headers_tgt SOURCES ${all_headers})
