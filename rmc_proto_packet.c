// Copyright (C) 2018, Jaguar Land Rover
// This program is licensed under the terms and conditions of the
// Mozilla Public License, version 2.0.  The full text of the
// Mozilla Public License is at https://www.mozilla.org/MPL/2.0/
//
// Author: Magnus Feuer (mfeuer1@jaguarlandrover.com)



#define _GNU_SOURCE
#include "rmc_proto.h"
#include <string.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>


int rmc_queue_packet(rmc_context_t* ctx,
                     void* payload,
                     payload_len_t payload_len)
{
    pub_packet_t *pack;

    if (!ctx || !payload || !payload_len)
        return EINVAL;
    
    pack = pub_next_queued_packet(&ctx->pub_ctx);    
 
    // FIXME: Upper limit to how many packets we can queue before
    //        returning ENOMEM
    pub_queue_packet(&ctx->pub_ctx, payload, payload_len, user_data_ptr(ctx));

    if (ctx->poll_modify)  {
        // Did we already have a packet pending for send prior
        // to queueing the lastest packet? If so, old action
        // was POLLWRITE, if not, it was 0.
        (*ctx->poll_modify)(ctx,
                            ctx->mcast_send_descriptor,
                            RMC_MULTICAST_SEND_INDEX,
                            (pack?RMC_POLLWRITE:0),
                            RMC_POLLWRITE);
    }

    return 0;
}


int rmc_get_dispatch_ready_count(rmc_context_t* ctx)
{
    return sub_get_dispatch_ready_count(&ctx->sub_ctx);
}

sub_packet_t* rmc_get_next_dispatch_ready(rmc_context_t* ctx)
{
    return sub_get_next_dispatch_ready(&ctx->sub_ctx);
}


int rmc_packet_dispatched(rmc_context_t* ctx, sub_packet_t* pack)
{
    rmc_connection_t* conn = sub_packet_user_data(pack).ptr;
    uint16_t old_action = 0;

    if (!conn)
        return EINVAL;
    
    sub_packet_dispatched(pack);

    // If this is the first packet that enters the ack-ready queue,
    // then we need to setup a timeout for when we gather 
    //
//    if (sub_get_acknowledge_ready_count(&ctx->sub_ctx) != 0)
//        ctx->next_send_ack = rmc_usec_monotonic_timestamp() + ctx->ack_timeout;
           
}


// Queue the packet up for being acked by rmc_proto_timeout.c::_process_packet_ack_timeout()
int rmc_packet_acknowledged(rmc_context_t* ctx, sub_packet_t* pack)
{
    rmc_connection_t* conn = 0;

    if (!conn || !pack)
        return EINVAL;
    
    conn = sub_packet_user_data(pack).ptr;
    if (conn->mode != RMC_CONNECTION_MODE_SUBSCRIBER)
        return EINVAL;

    sub_packet_acknowledged(pack);

    return 0;
}


rmc_connection_index_t rmc_sub_packet_connection(sub_packet_t* pack)
{
    rmc_connection_t* conn = 0;
    if (!pack)
        return 0;

    conn = (rmc_connection_t*) sub_packet_user_data(pack).ptr;

    return conn->connection_index;
}
