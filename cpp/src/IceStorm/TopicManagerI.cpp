// **********************************************************************
//
// Copyright (c) 2003
// ZeroC, Inc.
// Billerica, MA, USA
//
// All Rights Reserved.
//
// Ice is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License version 2 as published by
// the Free Software Foundation.
//
// **********************************************************************

#include <Ice/Ice.h>
#include <IceStorm/TopicManagerI.h>
#include <IceStorm/TopicI.h>
#include <IceStorm/Flusher.h>
#include <IceStorm/TraceLevels.h>

#include <functional>
#include <ctype.h>

using namespace IceStorm;
using namespace std;

TopicManagerI::TopicManagerI(const Ice::CommunicatorPtr& communicator, const Ice::ObjectAdapterPtr& topicAdapter,
                             const Ice::ObjectAdapterPtr& publishAdapter, const TraceLevelsPtr& traceLevels,
                             const Freeze::DBEnvironmentPtr& dbEnv, const Freeze::DBPtr& db) :
    _communicator(communicator),
    _topicAdapter(topicAdapter),
    _publishAdapter(publishAdapter),
    _traceLevels(traceLevels),
    _dbEnv(dbEnv),
    _topics(db)
{
    _flusher = new Flusher(_communicator, _traceLevels);
    _factory = new SubscriberFactory(_traceLevels, _flusher);

    //
    // Recreate each of the topics in the dictionary. If the topic
    // database doesn't exist then the topic was previously destroyed,
    // but not removed from the _topics dictionary. Normally this
    // should only occur in the event of a crash.
    //
    StringStringDict::iterator p = _topics.begin();
    while(p != _topics.end())
    {
	assert(_topicIMap.find(p->first) == _topicIMap.end());
	try
	{
	    installTopic("recreate", p->first, p->second, false);
	    ++p;
	}
	catch(const Freeze::DBNotFoundException& ex)
	{
	    if(_traceLevels->topicMgr > 0)
	    {
		Ice::Trace out(_traceLevels->logger, _traceLevels->topicMgrCat);
		out << ex;
	    }
	    StringStringDict::iterator tmp = p;
	    ++p;
	    _topics.erase(tmp);
	}
    }
}

TopicManagerI::~TopicManagerI()
{
}

TopicPrx
TopicManagerI::create(const string& name, const string& type, const Ice::Current&)
{
    validateType(type);

    // TODO: reader/writer mutex
    IceUtil::Mutex::Lock sync(*this);

    reap();

    if(_topicIMap.find(name) != _topicIMap.end())
    {
        TopicExists ex;
	ex.name = name;
        throw ex;
    }

    installTopic("create", name, type, true);
    _topics.insert(make_pair(name, type));

    //
    // The identity is the name of the Topic.
    //
    Ice::Identity id;
    id.name = name;
    return TopicPrx::uncheckedCast(_topicAdapter->createProxy(id));
}

TopicPrx
TopicManagerI::retrieve(const string& name, const Ice::Current&) const
{
    IceUtil::Mutex::Lock sync(*this);

    TopicManagerI* const This = const_cast<TopicManagerI* const>(this);
    This->reap();

    if(_topicIMap.find(name) != _topicIMap.end())
    {
	Ice::Identity id;
	id.name = name;
	return TopicPrx::uncheckedCast(_topicAdapter->createProxy(id));
    }

    NoSuchTopic ex;
    ex.name = name;
    throw ex;
}

//
// The arguments cannot be const & (for some reason)
//
struct TransformToTopicDict : public std::unary_function<TopicIMap::value_type, TopicDict::value_type>
{
    TransformToTopicDict(const Ice::ObjectAdapterPtr& adapter) :
        _adapter(adapter)
    {
    }

    TopicDict::value_type
    operator()(TopicIMap::value_type p)
    {
        Ice::Identity id;
        id.name = p.first;
        return TopicDict::value_type(p.first, TopicPrx::uncheckedCast(_adapter->createProxy(id)));
    }

    Ice::ObjectAdapterPtr _adapter;
};

TopicDict
TopicManagerI::retrieveAll(const Ice::Current&) const
{
    IceUtil::Mutex::Lock sync(*this);

    TopicManagerI* const This = const_cast<TopicManagerI* const>(this);
    This->reap();

    TopicDict all;
    transform(_topicIMap.begin(), _topicIMap.end(), inserter(all, all.begin()),
	      TransformToTopicDict(_topicAdapter));

    return all;
}

void
TopicManagerI::reap()
{
    //
    // Always Called with mutex locked
    //
    // IceUtil::Mutex::Lock sync(*this);
    //
    TopicIMap::iterator i = _topicIMap.begin();
    while(i != _topicIMap.end())
    {
	if(i->second->destroyed())
	{
	    if(_traceLevels->topicMgr > 0)
	    {
		Ice::Trace out(_traceLevels->logger, _traceLevels->topicMgrCat);
		out << "Reaping " << i->first;
	    }
	    _topics.erase(i->first);
	    _topicIMap.erase(i++);
	}
	else
	{
	    ++i;
	}
    }
}

void
TopicManagerI::installTopic(const string& message, const string& name, const string& type, bool create)
{
    if(_traceLevels->topicMgr > 0)
    {
	Ice::Trace out(_traceLevels->logger, _traceLevels->topicMgrCat);
	out << message << ' ' << name;
    }

    //
    // Prepend "topic-" to the topic name in order to form a
    // unique name for the Freeze database. Since the name we
    // supply to openDB is also used as a filename, we call
    // getDatabaseName to obtain a name with any questionable
    // filename characters converted to hex.
    //
    // TODO: instance
    // TODO: failure? cleanup database?
    //
    string dbName = "topic-" + getDatabaseName(name);
    Freeze::DBPtr db = _dbEnv->openDB(dbName, create);
    
    //
    // Create topic implementation
    //
    TopicIPtr topicI = new TopicI(_publishAdapter, _traceLevels, name, type, _factory, db);
    
    //
    // The identity is the name of the Topic.
    //
    Ice::Identity id;
    id.name = name;
    _topicAdapter->add(topicI, id);
    _topicIMap.insert(TopicIMap::value_type(name, topicI));
}

void
TopicManagerI::validateType(const string& type)
{
    bool fail = false;
    const string::size_type len = type.size();
    string::size_type pos = type.find("::");
    if(pos != 0)
    {
        fail = true;
    }

    bool checkAlpha = false;
    while(!fail && pos < len)
    {
        if(checkAlpha)
        {
            if(!isalpha(type[pos]))
            {
                fail = true;
            }
            else
            {
                checkAlpha = false;
            }
        }
        else if(type[pos] == ':')
        {
            pos++;
            if(pos == len || type[pos] != ':')
            {
                fail = true;
            }
            else
            {
                checkAlpha = true;
            }
        }
        else if(!isalnum(type[pos]))
        {
            fail = true;
        }
        pos++;
    }

    if(checkAlpha) // type ended with "::"
    {
        fail = true;
    }

    if(fail)
    {
        InvalidType ex;
        ex.type = type;
        throw ex;
    }
}

string
TopicManagerI::getDatabaseName(const string& name)
{
    string result;
    result.reserve(name.size());

    for(string::size_type i = 0; i < name.size(); i++)
    {
        if(isalnum(name[i]) || name[i] == '.' || name[i] == '-' || name[i] == '_')
        {
            result.push_back(name[i]);
        }
        else
        {
            ostringstream ostr;
            ostr << '%' << hex << static_cast<int>(name[i]);
            result.append(ostr.str());
        }
    }

    return result;
}
