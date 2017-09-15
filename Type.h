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
    enum class Kind
    {
        Basic,
        Composite,
        Enum,
        Set
    };

    explicit Type(xmlNodePtr node);
    virtual ~Type() noexcept;

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

protected:
    xmlNodePtr getNode() const
    {
        return m_node;
    }

    virtual Kind kindImpl() const = 0;
    virtual bool writeImpl(std::ostream& out, DB& db, unsigned indent) = 0;
    virtual std::size_t lengthImpl(DB& db) = 0;

    bool isDeperated(DB& db);
    bool isIntroduced(DB& db);

    void writeBrief(std::ostream& out, DB& db, unsigned indent);
    std::string nodeText();
    static std::size_t primitiveLength(const std::string& type);
    static std::pair<std::intmax_t, bool> stringToInt(const std::string& str);

private:
    enum Use
    {
        Use_Normal,
        Use_GroupSize,
        Use_Data,
        Use_NumOfValues
    };

    xmlNodePtr m_node = nullptr;
    std::array<unsigned, Use_NumOfValues> m_uses;
    XmlPropsMap m_props;
};

using TypePtr = std::unique_ptr<Type>;

} // namespace sbe2comms
