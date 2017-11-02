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

#include <vector>
#include <cstdint>
#include <cassert>

#include "cxxtest/TestSuite.h"
#include "comms/options.h"
#include "mine/Message.h"
#include "mine/MsgId.h"
#include "orig/MessageHeader.h"

namespace test
{

namespace common
{

using DataBuf = std::vector<std::uint8_t>;

class Handler
{
public:
    template <typename TMsg>
    void handle(TMsg& msg)
    {
        m_lastId = msg.doGetId();
    }

    void clear()
    {
        m_lastId = static_cast<mine::MsgId>(0);
    }

    mine::MsgId getLastId() const
    {
        return m_lastId;
    }

private:
    mine::MsgId m_lastId = static_cast<mine::MsgId>(0);
};

using MineMessageFull =
    mine::Message<
        comms::option::ReadIterator<const std::uint8_t*>,
        comms::option::WriteIterator<std::uint8_t*>,
        comms::option::IdInfoInterface,
        comms::option::LengthInfoInterface,
        comms::option::ValidCheckInterface,
        comms::option::Handler<Handler>
    >;

using MineMessageFullOutput =
    mine::Message<
        comms::option::ReadIterator<const std::uint8_t*>,
        comms::option::WriteIterator<std::back_insert_iterator<DataBuf> >,
        comms::option::IdInfoInterface,
        comms::option::LengthInfoInterface,
        comms::option::ValidCheckInterface,
        comms::option::Handler<Handler>
    >;

using MineMessageEmpty = mine::Message<>;

using MineMessageIdOnly =
    mine::Message<
        comms::option::IdInfoInterface
    >;

template <typename TMsg, typename TFrame>
DataBuf writeMsgIntoBuf(const TMsg& msg, const TFrame& frame)
{
    DataBuf buf;
    buf.resize(frame.length(msg));

    auto writeIter = comms::writeIteratorFor<TMsg>(&buf[0]);
    auto es = frame.write(msg, writeIter, buf.size());
    TS_ASSERT_EQUALS(es, comms::ErrorStatus::Success);
    auto diff =
        static_cast<std::size_t>(
            std::distance(comms::writeIteratorFor<TMsg>(&buf[0]), writeIter));
    TS_ASSERT_EQUALS(diff, buf.size());
    return buf;
}

template <typename TMsg, typename TFrame>
DataBuf writeMsgOutputIter(const TMsg& msg, const TFrame& frame)
{
    DataBuf buf;
    auto writeIter = std::back_inserter(buf);
    auto es = frame.write(msg, writeIter, buf.max_size());
    if (es == comms::ErrorStatus::UpdateRequired) {
        auto updateIter = &buf[0];
        es = frame.update(updateIter, buf.size());
    }
    TS_ASSERT_EQUALS(es, comms::ErrorStatus::Success);
    return buf;
}


template <typename TFrame>
typename TFrame::MsgPtr readMsg(const DataBuf& buf, TFrame& frame)
{
    typename TFrame::MsgPtr msgPtr;
    using Message = typename TFrame::MsgPtr::element_type;
    auto readIter = comms::readIteratorFor<Message>(&buf[0]);
    auto es = frame.read(msgPtr, readIter, buf.size());
    if (es != comms::ErrorStatus::Success) {
        TS_ASSERT(!msgPtr);
    }
    return msgPtr;
}

template <typename TOrig, typename TFrame>
TOrig wrapOrigMessage(DataBuf& buf, mine::MsgId expId)
{
    orig::MessageHeader hdr;
    auto offset = TFrame::Field::minLength();
    hdr.wrap((char*)&buf[0], offset, 0, buf.size());
    auto templateId = hdr.templateId();
    TS_ASSERT_EQUALS(expId, templateId);
    int actingVersion = hdr.version();
    int actingBlockLength = hdr.blockLength();
    auto headerLen =
        hdr.blockLengthEncodingLength() +
        hdr.templateIdEncodingLength() +
        hdr.schemaIdEncodingLength() +
        hdr.versionEncodingLength();
    orig::Msg1 origMsg;
    origMsg.wrapForDecode((char*)&buf[0], offset + headerLen, actingBlockLength, actingVersion, buf.size());
    return origMsg;
}

template <typename TMsg, typename TMsgInterface>
void verifyMine(const TMsg& msg1, TMsgInterface& msg2)
{
    test::common::Handler handler;
    msg2.dispatch(handler);
    TS_ASSERT_EQUALS(handler.getLastId(), msg1.doGetId());
    auto* msg2Ptr = static_cast<const TMsg*>(&msg2);
    TS_ASSERT_EQUALS(msg1, *msg2Ptr);
}

} // namespace common

} // namespace test
