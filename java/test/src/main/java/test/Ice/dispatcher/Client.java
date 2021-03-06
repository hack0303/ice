// **********************************************************************
//
// Copyright (c) 2003-2018 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

package test.Ice.dispatcher;

public class Client extends test.TestHelper
{
    public void run(String[] args)
    {
        com.zeroc.Ice.InitializationData initData = new com.zeroc.Ice.InitializationData();
        initData.properties = createTestProperties(args);
        initData.properties.setProperty("Ice.Package.Test", "test.Ice.dispatcher");

        assert(_dispatcher == null);
        _dispatcher = new Dispatcher();
        //
        // Limit the send buffer size, this test relies on the socket
        // send() blocking after sending a given amount of data.
        //
        initData.properties.setProperty("Ice.TCP.SndSize", "50000");

        initData.dispatcher = _dispatcher;
        try(com.zeroc.Ice.Communicator communicator = initialize(initData))
        {
            AllTests.allTests(this, _dispatcher);
        }
    }

    Dispatcher getDispatcher()
    {
        return _dispatcher;
    }

    //
    // The Dispatcher class uses a static "_instance" member in other language
    // mappings. In Java, we avoid the use of static members because we need to
    // maintain support for Android (in which the client and server run in the
    // same process).
    //
    private Dispatcher _dispatcher;
}
