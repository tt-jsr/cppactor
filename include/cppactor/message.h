/***************************************************************************
 *
 *                    Unpublished Work Copyright (c) 2014
 *                  Trading Technologies International, Inc.
 *                       All Rights Reserved Worldwide
 *
 *          * * *   S T R I C T L Y   P R O P R I E T A R Y   * * *
 *
 * WARNING:  This program (or document) is unpublished, proprietary property
 * of Trading Technologies International, Inc. and is to be maintained in
 * strict confidence. Unauthorized reproduction, distribution or disclosure
 * of this program (or document), or any program (or document) derived from
 * it is prohibited by State and Federal law, and by local law outside of
 * the U.S.
 *
 ***************************************************************************/
#pragma once
#include "cppactor/actor.h"
#include <memory>
#include <cassert>
#include <atomic>

namespace cppactor
{
    class message
    {
    public:
        message(int id);

        message(int id, actor_iptr& replyto);

        virtual ~message() {}

        actor_iptr get_reply_to();

        int msg_id;
    private:
        actor_iptr m_reply_to;
    };

    typedef std::unique_ptr<message> message_uptr;
} //cppactor
