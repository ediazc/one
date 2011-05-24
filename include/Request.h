/* -------------------------------------------------------------------------- */
/* Copyright 2002-2011, OpenNebula Project Leads (OpenNebula.org)             */
/*                                                                            */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may    */
/* not use this file except in compliance with the License. You may obtain    */
/* a copy of the License at                                                   */
/*                                                                            */
/* http://www.apache.org/licenses/LICENSE-2.0                                 */
/*                                                                            */
/* Unless required by applicable law or agreed to in writing, software        */
/* distributed under the License is distributed on an "AS IS" BASIS,          */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */
/* See the License for the specific language governing permissions and        */
/* limitations under the License.                                             */
/* -------------------------------------------------------------------------- */

#ifndef REQUEST_H_
#define REQUEST_H_

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>

#include "RequestManager.h"

using namespace std;


class Request: public xmlrpc_c::method
{
public:
    /**
     *  Wraps the actual execution function by authorizing the user
     *  and calling the request_execute virtual function
     *    @param _paramlist list of XML parameters
     *    @param _retval value to be returned to the client
     */
    virtual void execute(
        xmlrpc_c::paramList const& _paramList,
        xmlrpc_c::value *   const  _retval);

protected:

    Request(const string& mn, 
            const string& signature, 
            const string& help): method_name(mn), retval(0)
    {
        _signature = signature;
        _help      = help;
    };

    virtual ~Request(){};

    
    /**
     *  Actual Execution method for the request. Must be implemented by the
     *  XML-RPC requests
     *    @param uid of the user making the request
     *    @param gid of the user making the request
     *    @param _paramlist of the XML-RPC call (complete list)
     */
    virtual void request_execute(int uid, 
                                 int gid,
                                 xmlrpc_c::paramList const& _paramList) = 0;

    /**
     *  Builds an XML-RPC response updating retval. After calling this function
     *  the xml-rpc excute method should return
     *    @param val to be returned to the client
     */
    void success_response(int val);

    /**
     *  Builds an XML-RPC response updating retval. After calling this function
     *  the xml-rpc excute method should return
     *    @param val string to be returned to the client
     */
    void success_response(const string& val);

    /**
     *  Builds an XML-RPC response updating retval. After calling this function
     *  the xml-rpc excute method should return
     *    @param ec error code for this call
     *    @param val string representation of the error
     */
    void failure_response(RequestManager::ErrorCode ec, const string& val);

    /**
     *  Logs authorization errors
     *    @param action authorization action
     *    @param object object that needs to be authorized
     *    @param uid user that is authorized
     *    @param id id of the object, -1 for Pool
     *    @returns string for logging
     */
    string authorization_error (const string &action,
                                const string &object,
                                int   uid,
                                int   id);
    /**
     *  Logs authenticate errors
     *    @returns string for logging
     */
    string authenticate_error ();

    /**
     *  Logs get object errors
     *    @param object over which the get failed
     *    @param id of the object over which the get failed
     *    @returns string for logging
     */
    string get_error (const string &object,
                      int id);

    /**
     *  Logs action errors
     *    @param action that triggered the error
     *    @param object over which the action was applied
     *    @param id id of the object, -1 for Pool, -2 for no-id objects
     *              (allocate error, parse error)
     *    @param rc returned error code (NULL to ignore)
     *    @returns string for logging
     */
    string action_error (const string &action,
                         const string &object,
                         int id,
                         int rc);
private:
    /**
     *  The name of the XML-RPC method
     */
    string             method_name;

    /**
     *  Session token from the OpenNebula XML-RPC API
     */
    string             session;

    /**
     *  Return value of the request from libxmlrpc-c
     */
    xmlrpc_c::value * retval;
};

#endif //REQUEST_H_

