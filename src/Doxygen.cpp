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

#include "Doxygen.h"

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

namespace
{
const std::string DocDirName("doc");
} // namespace

Doxygen::Doxygen(DB& db)
  : m_db(db),
    m_name(db.getPackageName())
{
    ba::replace_all(m_name, " ", "_");
}

bool Doxygen::write()
{
    boost::system::error_code ec;
    auto dir = bf::path(m_db.getRootPath()) / DocDirName;
    bf::create_directories(dir, ec);
    if (ec) {
        log::error() << "Failed to create \"" << dir.string() <<
                "\" with error \"" << ec.message() << "\"!" << std::endl;
        return false;
    }


    return writeLayout() && writeConf() && writeNamespaces() && writeMain();
}

bool Doxygen::writeLayout()
{
    auto relPath = DocDirName + '/' + "layout.xml";
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    out << "<doxygenlayout version=\"1.0\">\n" <<
           "  <navindex>\n"
           "    <tab type=\"mainpage\" visible=\"yes\" title=\"\"/>\n"
           "    <tab type=\"pages\" visible=\"yes\" title=\"\" intro=\"\"/>\n"
           "    <tab type=\"modules\" visible=\"yes\" title=\"\" intro=\"\"/>\n"
           "    <tab type=\"namespaces\" visible=\"yes\" title=\"\">\n"
           "      <tab type=\"namespacelist\" visible=\"yes\" title=\"\" intro=\"\"/>\n"
           "      <tab type=\"namespacemembers\" visible=\"yes\" title=\"\" intro=\"\"/>\n"
           "    </tab>\n"
           "    <tab type=\"classes\" visible=\"yes\" title=\"\">\n"
           "      <tab type=\"classlist\" visible=\"yes\" title=\"\" intro=\"\"/>\n"
           "      <tab type=\"classindex\" visible=\"$ALPHABETICAL_INDEX\" title=\"\"/>\n"
           "      <tab type=\"hierarchy\" visible=\"yes\" title=\"\" intro=\"\"/>\n"
           "      <tab type=\"classmembers\" visible=\"yes\" title=\"\" intro=\"\"/>\n"
           "    </tab>\n"
           "    <tab type=\"files\" visible=\"yes\" title=\"\">\n"
           "      <tab type=\"filelist\" visible=\"yes\" title=\"\" intro=\"\"/>\n"
           "      <tab type=\"globals\" visible=\"yes\" title=\"\" intro=\"\"/>\n"
           "    </tab>\n"
           "    <tab type=\"examples\" visible=\"yes\" title=\"\" intro=\"\"/>\n"
           "  </navindex>\n\n"
           "  <!-- Layout definition for a class page -->\n"
           "  <class>\n"
           "    <includes visible=\"$SHOW_INCLUDE_FILES\"/>\n"
           "    <briefdescription visible=\"no\"/>\n"
           "    <detaileddescription title=\"\"/>\n"
           "    <inheritancegraph visible=\"$CLASS_GRAPH\"/>\n"
           "    <collaborationgraph visible=\"$COLLABORATION_GRAPH\"/>\n"
           "    <memberdecl>\n"
           "      <nestedclasses visible=\"yes\" title=\"\"/>\n"
           "      <publictypes title=\"\"/>\n"
           "      <services title=\"\"/>\n"
           "      <interfaces title=\"\"/>\n"
           "      <publicslots title=\"\"/>\n"
           "      <signals title=\"\"/>\n"
           "      <publicmethods title=\"\"/>\n"
           "      <publicstaticmethods title=\"\"/>\n"
           "      <publicattributes title=\"\"/>\n"
           "      <publicstaticattributes title=\"\"/>\n"
           "      <protectedtypes title=\"\"/>\n"
           "      <protectedslots title=\"\"/>\n"
           "      <protectedmethods title=\"\"/>\n"
           "      <protectedstaticmethods title=\"\"/>\n"
           "      <protectedattributes title=\"\"/>\n"
           "      <protectedstaticattributes title=\"\"/>\n"
           "      <packagetypes title=\"\"/>\n"
           "      <packagemethods title=\"\"/>\n"
           "      <packagestaticmethods title=\"\"/>\n"
           "      <packageattributes title=\"\"/>\n"
           "      <packagestaticattributes title=\"\"/>\n"
           "      <properties title=\"\"/>\n"
           "      <events title=\"\"/>\n"
           "      <privatetypes title=\"\"/>\n"
           "      <privateslots title=\"\"/>\n"
           "      <privatemethods title=\"\"/>\n"
           "      <privatestaticmethods title=\"\"/>\n"
           "      <privateattributes title=\"\"/>\n"
           "      <privatestaticattributes title=\"\"/>\n"
           "      <friends title=\"\"/>\n"
           "      <related title=\"\" subtitle=\"\"/>\n"
           "      <membergroups visible=\"yes\"/>\n"
           "    </memberdecl>\n"
           "    <memberdef>\n"
           "      <inlineclasses title=\"\"/>\n"
           "      <typedefs title=\"\"/>\n"
           "      <enums title=\"\"/>\n"
           "      <services title=\"\"/>\n"
           "      <interfaces title=\"\"/>\n"
           "      <constructors title=\"\"/>\n"
           "      <functions title=\"\"/>\n"
           "      <related title=\"\"/>\n"
           "      <variables title=\"\"/>\n"
           "      <properties title=\"\"/>\n"
           "      <events title=\"\"/>\n"
           "    </memberdef>\n"
           "    <allmemberslink visible=\"yes\"/>\n"
           "    <usedfiles visible=\"$SHOW_USED_FILES\"/>\n"
           "    <authorsection visible=\"yes\"/>\n"
           "  </class>\n\n"
           "  <namespace>\n"
           "    <briefdescription visible=\"yes\"/>\n"
           "    <memberdecl>\n"
           "      <nestednamespaces visible=\"yes\" title=\"\"/>\n"
           "      <constantgroups visible=\"yes\" title=\"\"/>\n"
           "      <classes visible=\"yes\" title=\"\"/>\n"
           "      <typedefs title=\"\"/>\n"
           "      <enums title=\"\"/>\n"
           "      <functions title=\"\"/>\n"
           "      <variables title=\"\"/>\n"
           "      <membergroups visible=\"yes\"/>\n"
           "    </memberdecl>\n"
           "    <detaileddescription title=\"\"/>\n"
           "    <memberdef>\n"
           "      <inlineclasses title=\"\"/>\n"
           "      <typedefs title=\"\"/>\n"
           "      <enums title=\"\"/>\n"
           "      <functions title=\"\"/>\n"
           "      <variables title=\"\"/>\n"
           "    </memberdef>\n"
           "    <authorsection visible=\"yes\"/>\n"
           "  </namespace>\n\n"
           "  <file>\n"
           "    <briefdescription visible=\"yes\"/>\n"
           "    <includes visible=\"$SHOW_INCLUDE_FILES\"/>\n"
           "    <includegraph visible=\"$INCLUDE_GRAPH\"/>\n"
           "    <includedbygraph visible=\"$INCLUDED_BY_GRAPH\"/>\n"
           "    <sourcelink visible=\"yes\"/>\n"
           "    <memberdecl>\n"
           "      <classes visible=\"yes\" title=\"\"/>\n"
           "      <namespaces visible=\"yes\" title=\"\"/>\n"
           "      <constantgroups visible=\"yes\" title=\"\"/>\n"
           "      <defines title=\"\"/>\n"
           "      <typedefs title=\"\"/>\n"
           "      <enums title=\"\"/>\n"
           "      <functions title=\"\"/>\n"
           "      <variables title=\"\"/>\n"
           "      <membergroups visible=\"yes\"/>\n"
           "    </memberdecl>\n"
           "    <detaileddescription title=\"\"/>\n"
           "    <memberdef>\n"
           "      <inlineclasses title=\"\"/>\n"
           "      <defines title=\"\"/>\n"
           "      <typedefs title=\"\"/>\n"
           "      <enums title=\"\"/>\n"
           "      <functions title=\"\"/>\n"
           "      <variables title=\"\"/>\n"
           "    </memberdef>\n"
           "    <authorsection/>\n"
           "  </file>\n\n"
           "  <group>\n"
           "    <briefdescription visible=\"yes\"/>\n"
           "    <groupgraph visible=\"$GROUP_GRAPHS\"/>\n"
           "    <memberdecl>\n"
           "      <nestedgroups visible=\"yes\" title=\"\"/>\n"
           "      <dirs visible=\"yes\" title=\"\"/>\n"
           "      <files visible=\"yes\" title=\"\"/>\n"
           "      <namespaces visible=\"yes\" title=\"\"/>\n"
           "      <classes visible=\"yes\" title=\"\"/>\n"
           "      <defines title=\"\"/>\n"
           "      <typedefs title=\"\"/>\n"
           "      <enums title=\"\"/>\n"
           "      <enumvalues title=\"\"/>\n"
           "      <functions title=\"\"/>\n"
           "      <variables title=\"\"/>\n"
           "      <signals title=\"\"/>\n"
           "      <publicslots title=\"\"/>\n"
           "      <protectedslots title=\"\"/>\n"
           "      <privateslots title=\"\"/>\n"
           "      <events title=\"\"/>\n"
           "      <properties title=\"\"/>\n"
           "      <friends title=\"\"/>\n"
           "      <membergroups visible=\"yes\"/>\n"
           "    </memberdecl>\n"
           "    <detaileddescription title=\"\"/>\n"
           "    <memberdef>\n"
           "      <pagedocs/>\n"
           "      <inlineclasses title=\"\"/>\n"
           "      <defines title=\"\"/>\n"
           "      <typedefs title=\"\"/>\n"
           "      <enums title=\"\"/>\n"
           "      <enumvalues title=\"\"/>\n"
           "      <functions title=\"\"/>\n"
           "      <variables title=\"\"/>\n"
           "      <signals title=\"\"/>\n"
           "      <publicslots title=\"\"/>\n"
           "      <protectedslots title=\"\"/>\n"
           "      <privateslots title=\"\"/>\n"
           "      <events title=\"\"/>\n"
           "      <properties title=\"\"/>\n"
           "      <friends title=\"\"/>\n"
           "    </memberdef>\n"
           "    <authorsection visible=\"yes\"/>\n"
           "  </group>\n\n"
           "  <directory>\n"
           "    <briefdescription visible=\"yes\"/>\n"
           "    <directorygraph visible=\"yes\"/>\n"
           "    <memberdecl>\n"
           "      <dirs visible=\"yes\"/>\n"
           "      <files visible=\"yes\"/>\n"
           "    </memberdecl>\n"
           "    <detaileddescription title=\"\"/>\n"
           "  </directory>\n"
           "</doxygenlayout>\n";
    return true;
}

bool Doxygen::writeConf()
{
    auto relPath = DocDirName + '/' + "doxygen.conf";
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    auto& pkgName = m_db.getPackageName();
    auto dirName = pkgName;
    ba::replace_all(dirName, " ", "_");

    out << "DOXYFILE_ENCODING      = UTF-8\n"
           "PROJECT_NAME           = \"" << pkgName << "\"\n"
           "PROJECT_BRIEF          = \"Documentation for " << pkgName << " project.\"\n"
           "OUTPUT_DIRECTORY = /" << dirName << "\n"
           "BRIEF_MEMBER_DESC      = YES\n"
           "REPEAT_BRIEF           = YES\n"
           "ALWAYS_DETAILED_SEC    = NO\n"
           "INLINE_INHERITED_MEMB  = YES\n"
           "FULL_PATH_NAMES        = YES\n"
           "SHORT_NAMES            = NO\n"
           "INHERIT_DOCS           = YES\n"
           "SEPARATE_MEMBER_PAGES  = NO\n"
           "TAB_SIZE               = 4\n"
           "OPTIMIZE_OUTPUT_FOR_C  = NO\n"
           "OPTIMIZE_OUTPUT_JAVA   = NO\n"
           "OPTIMIZE_FOR_FORTRAN   = NO\n"
           "OPTIMIZE_OUTPUT_VHDL   = NO\n"
           "MARKDOWN_SUPPORT       = YES\n"
           "AUTOLINK_SUPPORT       = YES\n"
           "BUILTIN_STL_SUPPORT    = YES\n"
           "CPP_CLI_SUPPORT        = NO\n"
           "SIP_SUPPORT            = NO\n"
           "IDL_PROPERTY_SUPPORT   = YES\n"
           "DISTRIBUTE_GROUP_DOC   = NO\n"
           "GROUP_NESTED_COMPOUNDS = NO\n"
           "SUBGROUPING            = YES\n"
           "INLINE_GROUPED_CLASSES = NO\n"
           "INLINE_SIMPLE_STRUCTS  = NO\n"
           "TYPEDEF_HIDES_STRUCT   = NO\n"
           "LOOKUP_CACHE_SIZE      = 0\n"
           "EXTRACT_ALL            = NO\n"
           "EXTRACT_PRIVATE        = NO\n"
           "EXTRACT_PACKAGE        = NO\n"
           "EXTRACT_STATIC         = NO\n"
           "EXTRACT_LOCAL_CLASSES  = YES\n"
           "EXTRACT_LOCAL_METHODS  = NO\n"
           "EXTRACT_ANON_NSPACES   = NO\n"
           "HIDE_UNDOC_MEMBERS     = YES\n"
           "HIDE_UNDOC_CLASSES     = YES\n"
           "HIDE_FRIEND_COMPOUNDS  = NO\n"
           "HIDE_IN_BODY_DOCS      = NO\n"
           "INTERNAL_DOCS          = NO\n"
           "CASE_SENSE_NAMES       = YES\n"
           "HIDE_SCOPE_NAMES       = NO\n"
           "HIDE_COMPOUND_REFERENCE= NO\n"
           "SHOW_INCLUDE_FILES     = YES\n"
           "SHOW_GROUPED_MEMB_INC  = NO\n"
           "FORCE_LOCAL_INCLUDES   = YES\n"
           "INLINE_INFO            = NO\n"
           "SORT_MEMBER_DOCS       = YES\n"
           "SORT_BRIEF_DOCS        = NO\n"
           "SORT_MEMBERS_CTORS_1ST = YES\n"
           "SORT_GROUP_NAMES       = NO\n"
           "SORT_BY_SCOPE_NAME     = YES\n"
           "STRICT_PROTO_MATCHING  = NO\n"
           "GENERATE_TODOLIST      = YES\n"
           "GENERATE_TESTLIST      = YES\n"
           "GENERATE_BUGLIST       = YES\n"
           "GENERATE_DEPRECATEDLIST= YES\n"
           "MAX_INITIALIZER_LINES  = 30\n"
           "SHOW_USED_FILES        = YES\n"
           "SHOW_FILES             = YES\n"
           "SHOW_NAMESPACES        = YES\n"
           "LAYOUT_FILE            = doc/layout.xml\n"
           "QUIET                  = YES\n"
           "WARNINGS               = YES\n"
           "WARN_IF_UNDOCUMENTED   = NO\n"
           "WARN_IF_DOC_ERROR      = YES\n"
           "WARN_NO_PARAMDOC       = YES\n"
           "WARN_AS_ERROR          = YES\n"
           "WARN_FORMAT            = \"$file:$line: $text\"\n"
           "INPUT_ENCODING         = UTF-8\n"
           "RECURSIVE              = YES\n"
           "EXCLUDE                = cc_plugin\n"
           "EXCLUDE_SYMLINKS       = NO\n"
           "EXCLUDE_PATTERNS       = */cc_plugin/* */install/*\n"
           "EXCLUDE_SYMBOLS        = *details *cc_plugin\n"
           "EXAMPLE_RECURSIVE      = NO\n"
           "FILTER_SOURCE_FILES    = NO\n"
           "SOURCE_BROWSER         = NO\n"
           "INLINE_SOURCES         = NO\n"
           "STRIP_CODE_COMMENTS    = YES\n"
           "REFERENCED_BY_RELATION = NO\n"
           "REFERENCES_RELATION    = NO\n"
           "REFERENCES_LINK_SOURCE = YES\n"
           "SOURCE_TOOLTIPS        = YES\n"
           "USE_HTAGS              = NO\n"
           "VERBATIM_HEADERS       = YES\n"
           "CLANG_ASSISTED_PARSING = NO\n"
           "CLANG_OPTIONS          =\n"
           "ALPHABETICAL_INDEX     = YES\n"
           "COLS_IN_ALPHA_INDEX    = 5\n"
           "GENERATE_HTML          = YES\n"
           "HTML_OUTPUT            = html\n"
           "HTML_FILE_EXTENSION    = .html\n"
           "HTML_COLORSTYLE_HUE    = 220\n"
           "HTML_COLORSTYLE_SAT    = 100\n"
           "HTML_COLORSTYLE_GAMMA  = 80\n"
           "HTML_TIMESTAMP         = YES\n"
           "HTML_DYNAMIC_SECTIONS  = NO\n"
           "HTML_INDEX_NUM_ENTRIES = 100\n"
           "GENERATE_DOCSET        = NO\n"
           "GENERATE_HTMLHELP      = NO\n"
           "GENERATE_QHP           = NO\n"
           "GENERATE_ECLIPSEHELP   = NO\n"
           "DISABLE_INDEX          = NO\n"
           "GENERATE_TREEVIEW      = NO\n"
           "ENUM_VALUES_PER_LINE   = 4\n"
           "TREEVIEW_WIDTH         = 250\n"
           "EXT_LINKS_IN_WINDOW    = NO\n"
           "FORMULA_FONTSIZE       = 10\n"
           "FORMULA_TRANSPARENT    = YES\n"
           "USE_MATHJAX            = NO\n"
           "SEARCHENGINE           = NO\n"
           "SERVER_BASED_SEARCH    = NO\n"
           "EXTERNAL_SEARCH        = NO\n"
           "SEARCHDATA_FILE        = searchdata.xml\n"
           "GENERATE_LATEX         = NO\n"
           "GENERATE_RTF           = NO\n"
           "GENERATE_MAN           = NO\n"
           "GENERATE_XML           = NO\n"
           "GENERATE_DOCBOOK       = NO\n"
           "GENERATE_AUTOGEN_DEF   = NO\n"
           "GENERATE_PERLMOD       = NO\n"
           "ENABLE_PREPROCESSING   = YES\n"
           "MACRO_EXPANSION        = NO\n"
           "EXPAND_ONLY_PREDEF     = NO\n"
           "SEARCH_INCLUDES        = YES\n"
           "PREDEFINED             = FOR_DOXYGEN_DOC_ONLY\n"
           "SKIP_FUNCTION_MACROS   = YES\n"
           "ALLEXTERNALS           = NO\n"
           "EXTERNAL_GROUPS        = YES\n"
           "EXTERNAL_PAGES         = YES\n"
           "PERL_PATH              = /usr/bin/perl\n"
           "CLASS_DIAGRAMS         = YES\n"
           "HIDE_UNDOC_RELATIONS   = YES\n"
           "HAVE_DOT               = NO\n";

    return true;
}

bool Doxygen::writeNamespaces()
{
    auto& ns = m_db.getProtocolNamespace();
    auto nsFile = ns + ".dox";
    if (ns.empty()) {
        nsFile = "namespaces" + nsFile;
    }

    auto relPath = DocDirName + '/' + nsFile;
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    if (!ns.empty()) {
        out << "/// \\namespace " << ns << "\n"
               "/// \\brief Main namespace for all classes / functions of this protocol library.\n\n";
    }

    out << "/// \\namespace " << common::scopeFor(ns, common::messageNamespaceNameStr()) << "\n"
           "/// \\brief Namespace for all the messages defined in this protocol.\n\n"
           "/// \\namespace " << common::scopeFor(ns, common::fieldNamespaceNameStr()) << "\n"
           "/// \\brief Namespace for all the stand alone fields defined in this protocol.\n\n"
           "/// \\namespace " << common::builtinNamespaceNameStr() << "\n"
           "/// \\brief Namespace for all implicitly defined (built-in) fields.\n\n";
    return true;
}

bool Doxygen::writeMain()
{
    auto relPath = DocDirName + "/main.dox";
    auto filePath = bf::path(m_db.getRootPath()) / relPath;
    log::info() << "Generating " << relPath << std::endl;
    std::ofstream out(filePath.string());
    if (!out) {
        log::error() << "Failed to create " << filePath.string() << std::endl;
        return false;
    }

    out << "/// \\mainpage " << m_db.getPackageName() << " Binary Protocol\n"
           "/// \\tableofcontents\n"
           "/// \\section main_paige_overview Overview\n"
           "/// TODO\n"
           "///\n\n";
    return true;
}




} // namespace sbe2comms
