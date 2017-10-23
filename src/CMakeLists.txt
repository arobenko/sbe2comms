
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
    "DB.cpp"
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
    "common.cpp"
    "prop.cpp"
    "output.cpp"
    "log.cpp"
)

add_executable(${PROJECT_NAME} ${src})
target_link_libraries(${PROJECT_NAME} ${LIBXML2_LIBRARIES} ${Boost_LIBRARIES})


FILE(GLOB_RECURSE all_headers "*.h")
add_custom_target(all_headers_tgt SOURCES ${all_headers})
