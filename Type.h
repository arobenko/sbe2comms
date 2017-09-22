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

#pragma once

#include <memory>
#include <array>
#include <iosfwd>
#include <cstdint>
#include <utility>

#include "xml_wrap.h"
#include "prop.h"

namespace sbe2comms
{

class DB;
class Type
{
public:
    using Ptr = std::unique_ptr<Type>;

    enum class Kind
    {
        Basic,
        Composite,
        Enum,
        Set,
        Ref
    };

    Type(DB& db, xmlNodePtr node);
    virtual ~Type() noexcept;

    bool parse();

    const char* getNodeName() const;

    const std::string& getName() const;

    static Ptr create(const std::string& name, DB& db, xmlNodePtr node);

    bool normalUseRecorded() const
    {
        return 0U < m_uses[Use_Normal];
    }

    bool groupSizeUseRecorded() const
    {
        return 0U < m_uses[Use_GroupSize];
    }

    bool dataUseRecorded() const
    {
        return 0U < m_uses[Use_Data];
    }

    void recordNormalUse()
    {
        ++m_uses[Use_Normal];
    }

    void recordGroupSizeUse()
    {
        ++m_uses[Use_GroupSize];
    }

    void recordDataUse()
    {
        ++m_uses[Use_Data];
    }

    Kind kind() const
    {
        return kindImpl();
    }

    bool write(std::ostream& out, DB& db, unsigned indent = 0);

    const XmlPropsMap& props(DB& db);

    std::size_t length(DB& db)
    {
        return lengthImpl(db);
    }

    bool writeDependencies(std::ostream& out, DB& db, unsigned indent = 0)
    {
        return writeDependenciesImpl(out, db, indent);
    }

protected:
    xmlNodePtr getNode() const
    {
        return m_node;
    }

    DB& getDb()
    {
        return m_db;
    }

    virtual Kind kindImpl() const = 0;
    virtual bool parseImpl();
    virtual bool writeImpl(std::ostream& out, DB& db, unsigned indent) = 0;
    virtual std::size_t lengthImpl(DB& db) = 0;
    virtual bool writeDependenciesImpl(std::ostream& out, DB& db, unsigned indent);

    bool isDeperated(DB& db);
    bool isIntroduced(DB& db);

    void writeBrief(std::ostream& out, DB& db, unsigned indent, bool extraOpts = false);
    std::string nodeText();
    static std::size_t primitiveLength(const std::string& type);
    static std::pair<std::intmax_t, bool> stringToInt(const std::string& str);
    static const std::string& primitiveTypeToStdInt(const std::string& type);
    static std::pair<std::intmax_t, bool> intMinValue(const std::string& type, const std::string& value);
    static std::pair<std::intmax_t, bool> intMaxValue(const std::string& type, const std::string& value);
    static std::intmax_t builtInIntNullValue(const std::string& type);
    static std::string toString(std::intmax_t val);


private:
    enum Use
    {
        Use_Normal,
        Use_GroupSize,
        Use_Data,
        Use_NumOfValues
    };

    DB& m_db;
    xmlNodePtr m_node = nullptr;
    std::array<unsigned, Use_NumOfValues> m_uses;
    XmlPropsMap m_props;
    bool m_written = false;
    bool m_writingInProgress = false;
};

using TypePtr = Type::Ptr;

} // namespace sbe2comms
