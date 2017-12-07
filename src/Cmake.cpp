//
// Copyright 2017 (C). Alex Robenko. All rights reserved.
//

// This code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "Cmake.h"

#include <iostream>
#include <fstream>
//#include <algorithm>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "common.h"
#include "output.h"
#include "DB.h"
#include "log.h"

namespace bf = boost::filesystem;
namespace ba = boost::algorithm;

namespace sbe2comms
{

Cmake::Cmake(DB& db)
  : m_db(db),
    m_name(db.getPackageName())
{
    ba::replace_all(m_name, " ", "_");
}

bool Cmake::write()
{
    return writeMain() && writePlugin();
}

bool Cmake::writeMain()
{
    boost::system::error_code ec;
    bf::create_directories(m_db.getRootPath(), ec);
    if (ec) {
        log::error() << "Failed to create \"" << m_db.getRootPath() <<
                "\" with error \"" << ec.message() << "\"!" << std::endl;
        return false;
    }

    auto relPath = common::cmakeListsFileName();
    auto filePath = bf::path( m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    out << "cmake_minimum_required (VERSION 3.1)\n"
           "project (\"" << m_name << "\")\n\n"
           "option (OPT_LIB_ONLY \"Install only protocol library, no other libraries/plugings are built/installed.\" OFF)\n"
           "option (OPT_THIS_AND_COMMS_LIBS_ONLY \"Install this protocol and COMMS libraries only, no other applications/plugings are built/installed.\" OFF)\n"
           "option (OPT_FULL_SOLUTION \"Build and install full solution, including CommsChampion sources.\" ON)\n"
           "option (OPT_NO_WARN_AS_ERR \"Do NOT treat warning as error\" OFF)\n\n"
           "# Other parameters:\n"
           "# OPT_INSTALL_DIR - Custom install directory.\n"
           "# OPT_QT_DIR - Path to custom Qt5 install directory.\n"
           "# OPT_CC_MAIN_INSTALL_DIR - Path to CommsChampion install directory (if such already built).\n"
           "\n"
           "if (NOT CMAKE_CXX_STANDARD)\n" <<
           output::indent(1) << "set (CMAKE_CXX_STANDARD 11)\n"
           "endif()\n\n"
           "set (INSTALL_DIR ${CMAKE_BINARY_DIR}/install)\n"
           "if (NOT \"${OPT_INSTALL_DIR}\" STREQUAL \"\")\n" <<
           output::indent(1) << "set (INSTALL_DIR \"${OPT_INSTALL_DIR}\")\n"
           "endif ()\n\n"
           "include(GNUInstallDirs)\n"
           "set (LIB_INSTALL_DIR ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR})\n"
           "set (BIN_INSTALL_DIR ${INSTALL_DIR}/${CMAKE_INSTALL_BINDIR})\n"
           "set (INC_INSTALL_DIR ${INSTALL_DIR}/${CMAKE_INSTALL_INCLUDEDIR})\n"
           "set (CONFIG_INSTALL_DIR ${INSTALL_DIR}/config)\n"
           "set (PLUGIN_INSTALL_DIR ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/CommsChampion/plugin)\n"
           "set (DOC_INSTALL_DIR ${INSTALL_DIR}/doc)\n"
           "\n"
           "install (\n" <<
           output::indent(1) << "DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ublox\n" <<
           output::indent(1) << "DESTINATION ${INC_INSTALL_DIR}\n"
           ")\n\n"
           "FILE(GLOB_RECURSE protocol.headers \"include/*.h\")\n"
           "add_custom_target(" << m_name << ".headers SOURCES ${protocol.headers})\n\n"
           "find_package (Doxygen)\n"
           "if (DOXYGEN_FOUND)\n" <<
           output::indent(1) << "# TODO:\n"
           "endif ()\n\n"
           "if (OPT_LIB_ONLY)\n" <<
           output::indent(1) << "return ()\n"
           "endif ()\n\n"
           "######################################################################\n\n"
           "set (CC_EXTERNAL_TGT \"comms_champion_external\")\n"
           "include(ExternalProject)\n"
           "macro (externals install_dir build_cc)\n" <<
           output::indent(1) << "set (cc_tag \"" << m_db.getCommsChampionTag() << "\")\n" <<
           output::indent(1) << "set (cc_main_dir \"${CMAKE_BINARY_DIR}/comms_champion\")\n" <<
           output::indent(1) << "set (cc_src_dir \"${cc_main_dir}/src\")\n" <<
           output::indent(1) << "set (cc_bin_dir \"${cc_main_dir}/build\")\n\n" <<
           output::indent(1) << "if (NOT \"${OPT_QT_DIR}\" STREQUAL \"\")\n" <<
           output::indent(2) << "set (cc_qt_dir_opt -DCC_QT_DIR=${OPT_QT_DIR})\n" <<
           output::indent(1) << "endif ()\n\n" <<
           output::indent(1) << "if (${build_cc})\n" <<
           output::indent(2) << "set (CC_PLUGIN_LIBRARIES \"comms_champion\")\n" <<
           output::indent(2) << "set (CC_COMMS_CHAMPION_FOUND TRUE)\n" <<
           output::indent(2) << "set (CC_PLUGIN_LIBRARY_DIRS ${LIB_INSTALL_DIR})\n" <<
           output::indent(1) << "else ()\n" <<
           output::indent(2) << "set (ct_lib_only_opt -DCC_COMMS_LIB_ONLY=ON)\n" <<
           output::indent(1) << "endif ()\n\n" <<
           output::indent(1) << "ExternalProject_Add(\n" <<
           output::indent(2) << "\"${CC_EXTERNAL_TGT}\"\n" <<
           output::indent(2) << "PREFIX \"${cc_bin_dir}\"\n" <<
           output::indent(2) << "STAMP_DIR \"${cc_bin_dir}\"\n" <<
           output::indent(2) << "GIT_REPOSITORY \"https://github.com/arobenko/comms_champion.git\"\n" <<
           output::indent(2) << "GIT_TAG \"${cc_tag}\"\n" <<
           output::indent(2) << "SOURCE_DIR \"${cc_src_dir}\"\n" <<
           output::indent(2) << "CMAKE_ARGS\n" <<
           output::indent(3) << "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCC_INSTALL_DIR=${install_dir}\n" <<
           output::indent(3) << "-DCC_NO_UNIT_TESTS=ON -DCC_NO_WARN_AS_ERR=ON -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}\n" <<
           output::indent(3) << "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}\n" <<
           output::indent(3) << "${cc_qt_dir_opt} ${ct_lib_only_opt}\n" <<
           output::indent(2) << "BINARY_DIR \"${cc_bin_dir}\"\n" <<
           output::indent(1) << ")\n\n" <<
           output::indent(1) << "set (CC_EXTERNAL TRUE)\n" <<
           output::indent(1) << "set (CC_COMMS_FOUND TRUE)\n" <<
           output::indent(1) << "set (CC_CMAKE_DIR ${LIB_INSTALL_DIR}/cmake)\n\n" <<
           output::indent(1) << "include_directories(\"${install_dir}/${CMAKE_INSTALL_INCLUDEDIR}\")\n" <<
           output::indent(1) << "link_directories(\"${install_dir}/${CMAKE_INSTALL_LIBDIR}\")\n" <<
           "\n"
           "endmacro()\n\n"
           "######################################################################\n\n"
           "if (OPT_THIS_AND_COMMS_LIBS_ONLY)\n" <<
           output::indent(1) << "externals(${INSTALL_DIR} FALSE)\n" <<
           output::indent(1) << "return()\n" <<
           "endif ()\n\n"
           "while (TRUE)\n" <<
           output::indent(1) << "if (OPT_FULL_SOLUTION)\n" <<
           output::indent(2) << "externals(${INSTALL_DIR} TRUE)\n" <<
           output::indent(2) << "break()\n" <<
           output::indent(1) << "endif ()\n\n" <<
           output::indent(1) << "list (APPEND CMAKE_PREFIX_PATH \"${INSTALL_DIR}\")\n" <<
           output::indent(1) << "if (NOT \"${OPT_CC_MAIN_INSTALL_DIR}\" STREQUAL \"\")\n" <<
           output::indent(2) << "list (APPEND CMAKE_PREFIX_PATH \"${OPT_CC_MAIN_INSTALL_DIR}\")\n" <<
           output::indent(1) << "endif ()\n\n" <<
           output::indent(1) << "find_package(CommsChampion QUIET NO_MODULE)\n\n" <<
           output::indent(1) << "if (NOT CC_COMMS_FOUND)\n" <<
           output::indent(2) << "set (externals_install \"${CMAKE_BINARY_DIR}/ext_install\")\n" <<
           output::indent(2) << "set (build_cc FALSE)\n" <<
           output::indent(2) << "if ((NOT OPT_LIB_ONLY) AND (NOT OPT_THIS_AND_COMMS_LIBS_ONLY))\n" <<
           output::indent(3) << "set (build_cc TRUE)\n" <<
           output::indent(2) << "endif ()\n\n" <<
           output::indent(2) << "externals(${externals_install} ${build_cc})\n" <<
           output::indent(2) << "break()\n" <<
           output::indent(1) << "endif ()\n\n" <<
           output::indent(1) << "find_package(CommsChampion NO_MODULE)\n" <<
           output::indent(1) << "if (CC_COMMS_FOUND)\n" <<
           output::indent(2) << "include_directories(${CC_INCLUDE_DIRS})\n" <<
           output::indent(1) << "endif ()\n\n" <<
           output::indent(1) << "if (CC_COMMS_CHAMPION_FOUND)\n" <<
           output::indent(2) << "link_directories(${CC_PLUGIN_LIBRARY_DIRS})\n" <<
           output::indent(2) << "file (RELATIVE_PATH rel_plugin_install_path \"${CC_ROOT_DIR}\" \"${CC_PLUGIN_DIR}\")\n" <<
           output::indent(2) << "set (PLUGIN_INSTALL_DIR \"${INSTALL_DIR}/${rel_plugin_install_path}\")\n" <<
           output::indent(1) << "endif ()\n\n" <<
           output::indent(1) << "if (EXISTS \"${INSTALL_DIR}/cmake/CommsChampionConfig.cmake\")\n" <<
           output::indent(2) << "FILE(GLOB_RECURSE comms.headers \"${INSTALL_DIR}/include/comms/*.h\")\n" <<
           output::indent(2) << "add_custom_target(comms.headers SOURCES ${comms.headers})\n" <<
           output::indent(2) << "FILE(GLOB_RECURSE cc.headers \"${INSTALL_DIR}/include/comms_champion/*.h\")\n" <<
           output::indent(2) << "add_custom_target(cc.headers SOURCES ${cc.headers})\n" <<
           output::indent(1) << "endif ()\n\n" <<
           output::indent(1) << "break()\n"
           "endwhile()\n\n"
           "if (NOT \"${OPT_QT_DIR}\" STREQUAL \"\")\n" <<
           output::indent(1) << "list (APPEND CMAKE_PREFIX_PATH ${OPT_QT_DIR})\n" <<
           "endif ()\n\n"
           "find_package(Qt5Core)\n\n"
           "if ((CMAKE_COMPILER_IS_GNUCC) OR (\"${CMAKE_CXX_COMPILER_ID}\" STREQUAL \"Clang\"))\n" <<
           output::indent(1) << "set (extra_flags_list\n" <<
           output::indent(2) << "\"-Wall\" \"-Wextra\" \"-Wcast-align\" \"-Wcast-qual\" \"-Wctor-dtor-privacy\"\n" <<
           output::indent(2) << "\"-Wmissing-include-dirs\"\n" <<
           output::indent(2) << "\"-Woverloaded-virtual\" \"-Wredundant-decls\" \"-Wshadow\" \"-Wundef\" \"-Wunused\"\n" <<
           output::indent(2) << "\"-Wno-unknown-pragmas\" \"-fdiagnostics-show-option\"\n" <<
           output::indent(1) << ")\n\n" <<
           output::indent(1) << "if (CMAKE_COMPILER_IS_GNUCC)\n" <<
           output::indent(2) << "list (APPEND extra_flags_list\n" <<
           output::indent(3) << "\"-Wnoexcept\" \"-Wlogical-op\" \"-Wstrict-null-sentinel\"\n" <<
           output::indent(2) << ")\n" <<
           output::indent(1) << "endif ()\n\n" <<
           output::indent(1) << "if (\"${CMAKE_CXX_COMPILER_ID}\" STREQUAL \"Clang\")\n" <<
           output::indent(2) << "list (APPEND extra_flags_list \"-Wno-dangling-field\" \"-Wno-unused-command-line-argument\" \"-ftemplate-depth=1024\")\n" <<
           output::indent(1) << "endif ()\n\n" <<
           output::indent(1) << "if (NOT OPT_NO_WARN_AS_ERR)\n" <<
           output::indent(2) << "list (APPEND extra_flags_list \"-Werror\")\n" <<
           output::indent(1) << "endif ()\n\n" <<
           output::indent(1) << "string(REPLACE \";\" \" \" extra_flags \"${extra_flags_list}\")\n" <<
           output::indent(1) << "set (CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} ${extra_flags}\")\n" <<
           "elseif (MSVC)\n" <<
           output::indent(1) << "add_definitions( \"/wd4503\" \"/wd4309\" \"/wd4267\" \"-D_SCL_SECURE_NO_WARNINGS\")\n" <<
           output::indent(1) << "if (NOT CC_NO_WARN_AS_ERR)\n" <<
           output::indent(2) << "add_definitions(\"/WX\")\n" <<
           output::indent(1) << "endif ()\n"
           "endif ()\n\n"
           "include_directories(\n" <<
           output::indent(1) << "BEFORE\n" <<
           output::indent(1) << "${CMAKE_SOURCE_DIR}\n" <<
           output::indent(1) << "${CMAKE_SOURCE_DIR}/include\n"
           ")\n\n"
           "add_subdirectory(cc_plugin)\n\n";

    return true;
}

bool Cmake::writePlugin()
{
    boost::system::error_code ec;
    bf::create_directories(m_db.getRootPath(), ec);
    if (!common::createPluginDefDir(m_db.getRootPath(), common::pluginNamespaceNameStr())) {
        return false;
    }

    auto relPath = common::pluginNamespaceNameStr() + '/' + common::cmakeListsFileName();
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    out << "set (ALL_MESSAGES_LIB \"all_messages\")\n\n"
           "######################################################################\n\n"
           "function (cc_plugin_all_messages)\n" <<
           output::indent(1) << "set (name \"${ALL_MESSAGES_LIB}\")\n\n" <<
           output::indent(1) << "set (src\n" <<
           output::indent(2) << common::fieldDefFileName() << '\n';
    auto& msgs = m_db.getMessagesById();
    for (auto& m : msgs) {
        assert(m.second != m_db.getMessages().end());
        out << output::indent(2) << common::messageDirName() << '/' << m.second->first << ".cpp\n";
    }
    out << output::indent(1) << ")\n\n" <<
           output::indent(1) << "add_library (${name} STATIC ${src})\n" <<
           output::indent(1) << "target_link_libraries (${name} ${CC_PLUGIN_LIBRARIES})\n" <<
           output::indent(1) << "qt5_use_modules(${name} Core)\n"
           "endfunction()\n\n"
           "######################################################################\n\n"
           "if (NOT Qt5Core_FOUND)\n" <<
           output::indent(1) << "message (WARNING \"Can NOT compile protocol plugin due to missing QT5 Core library\")\n" <<
           output::indent(1) << "return ()\n"
           "endif ()\n\n"
           "if (CMAKE_COMPILER_IS_GNUCC)\n" <<
           output::indent(1) << "set (CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} -ftemplate-backtrace-limit=0\")\n"
           "endif ()\n\n" <<
           "cc_plugin_all_messages()\n"
           "FILE(GLOB_RECURSE plugin.headers \"*.h\")\n"
           "add_custom_target(cc_plugin.headers SOURCES ${plugin.headers})\n\n";

    return true;
}


} // namespace sbe2comms
