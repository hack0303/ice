// **********************************************************************
//
// Copyright (c) 2003-2005 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#ifndef ICE_CONNECTION_ICE
#define ICE_CONNECTION_ICE

#include <Ice/ObjectAdapterF.ice>
#include <Ice/Identity.ice>

module Ice
{

/**
 *
 * The user-level interface to a connection.
 *
 **/
local interface Connection
{
    /**
     *
     * Close a connection, either gracefully or forcefully. If a
     * connection is closed forcefully, it closes immediately, without
     * sending the relevant close connection protocol messages to the
     * peer and waiting for the peer to acknowledge these protocol
     * messages.
     *
     * @param force If true, close forcefully. Otherwise the
     * connection is closed gracefully.
     *
     **/
    void close(bool force);

    /**
     *
     * Create a special proxy that always uses this connection. This
     * can be used for callbacks from a server to a client if the
     * server cannot directly establish a connection to the client,
     * for example because of firewalls. In this case, the server
     * would create a proxy using an already established connection
     * from the client.
     *
     * @param id The identity for which a proxy is to be created.
     *
     * @return A proxy that matches the given identity and uses this
     * connection.
     *
     * @see setAdapter
     *
     **/
    nonmutating Object* createProxy(Identity id);

    /**
     *
     * Explicitly set an object adapter that dispatches requests that
     * are received over this connection. A client can invoke an
     * operation on a server using a proxy, and then set an object
     * adapter for the outgoing connection that is used by the proxy
     * in order to receive callbacks. This is useful if the server
     * cannot establish a connection back to the client, for example
     * because of firewalls.
     *
     * @param adapter The object adapter that should be used by this
     * connection to dispatch requests. The object adapter must be
     * activated. When the object adapter is deactivated, it is
     * automatically removed from the connection.
     *
     * @see createProxy
     * @see setAdapter
     *
     **/
    void setAdapter(ObjectAdapter adapter);

    /**
     *
     * Get the object adapter that dispatches requests for this
     * connection.
     *
     * @return The object adapter that dispatches requests for the
     * connection, or null if no adapter is set.
     *
     * @see setAdapter
     *
     **/
    nonmutating ObjectAdapter getAdapter();

    /**
     *
     * Flush any pending batch requests for this connection. This
     * causes all batch requests that were sent via proxies that use
     * this connection to be sent to the server.
     *
     **/
    void flushBatchRequests();

    /**
     *
     * Return the connection type. This corresponds to the endpoint
     * type, i.e., "tcp", "udp", etc.
     *
     * @return The type of the connection.
     *
     **/
    nonmutating string type();

    /**
     *
     * Get the timeout for the connection.
     *
     * @return The connection's timeout.
     *
     **/
    nonmutating int timeout();

    /**
     *
     * Return a description of the connection as human readable text,
     * suitable for logging or error messages.
     *
     * @return The description of the connection as human readable
     * text.
     *
     **/
    nonmutating string toString();
};

};

#endif
