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
#include "cppactor/actor.h"
#include "cppactor/message.h"

namespace cppactor
{
    message::message(int id)
    : msg_id(id)
    {}

    message::message(int id, actor_iptr& replyto)
    : msg_id(id)
    , m_reply_to(replyto)
    {}


    actor_iptr message::get_reply_to() 
    {
        return m_reply_to;
    }

} //cppactor
