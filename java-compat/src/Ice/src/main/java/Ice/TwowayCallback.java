// **********************************************************************
//
// Copyright (c) 2003-2018 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

package Ice;

/**
 * Base interface for generated twoway operation callback.
 **/
public interface TwowayCallback
{
    /**
     * Called when the invocation raises an Ice run-time exception.
     *
     * @param ex The Ice run-time exception raised by the operation.
     **/
    public void exception(LocalException ex);

    /**
     * Called when the invocation raises an Ice system exception.
     *
     * @param ex The Ice system exception raised by the operation.
     **/
    public void exception(SystemException ex);
}
