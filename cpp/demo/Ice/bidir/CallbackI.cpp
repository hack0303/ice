// **********************************************************************
//
// Copyright (c) 2003-2005 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <Ice/Ice.h>
#include <CallbackI.h>

using namespace std;
using namespace Ice;
using namespace Demo;

CallbackSenderI::CallbackSenderI() :
    _destroy(false),
    _num(0)
{
}

void
CallbackSenderI::destroy()
{
    {
	IceUtil::Monitor<IceUtil::Mutex>::Lock lock(*this);
	
	cout << "destroying callback sender" << endl;
	_destroy = true;
	
	notify();
    }

    getThreadControl().join();
}

void
CallbackSenderI::addClient(const Identity& ident, const Current& current)
{
    IceUtil::Monitor<IceUtil::Mutex>::Lock lock(*this);

    cout << "adding client `" << identityToString(ident) << "'"<< endl;

    CallbackReceiverPrx client = CallbackReceiverPrx::uncheckedCast(current.con->createProxy(ident));
    _clients.insert(client);
}

void
CallbackSenderI::run()
{
    IceUtil::Monitor<IceUtil::Mutex>::Lock lock(*this);

    while(!_destroy)
    {
	timedWait(IceUtil::Time::seconds(2));

	if(!_destroy && !_clients.empty())
	{
	    ++_num;
	    
	    set<CallbackReceiverPrx>::iterator p = _clients.begin();
	    while(p != _clients.end())
	    {
		try
		{
		    (*p)->callback(_num);
		    ++p;
		}
		catch(const Exception& ex)
		{
		    cerr << "removing client `" << identityToString((*p)->ice_getIdentity()) << "':\n"
			 << ex << endl;
		    _clients.erase(p++);
		}
	    }
	}
    }
}
