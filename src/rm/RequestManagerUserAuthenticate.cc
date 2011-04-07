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

#include "RequestManager.h"
#include "NebulaLog.h"

#include "AuthManager.h"

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void RequestManager::UserAuthenticate::execute(
    xmlrpc_c::paramList const& paramList,
    xmlrpc_c::value *   const  retval)
{
    string          session;
    int             uid;

    ostringstream   oss;
    const string    method_name = "UserAuthenticate";


    /*   -- RPC specific vars --  */
    vector<xmlrpc_c::value> arrayData;  
    xmlrpc_c::value_array * arrayresult;

    NebulaLog::log("ReM",Log::DEBUG,"UserAuthenticate method invoked");

    // Get the parameters
    session  = xmlrpc_c::value_string(paramList.getString(0));

    // Try to authenticate the user
    uid = UserAuthenticate::upool->authenticate(session);

    if( uid == -1 )
    {
        goto error_common;
    }

    //Result
    arrayData.push_back(xmlrpc_c::value_boolean( true ));
    arrayData.push_back(xmlrpc_c::value_int(uid));
    arrayresult = new xmlrpc_c::value_array(arrayData);

    *retval = *arrayresult;

    delete arrayresult;

    return;

error_common:
    oss.str(authenticate_error(method_name));

    arrayData.push_back(xmlrpc_c::value_boolean(false));  // FAILURE
    arrayData.push_back(xmlrpc_c::value_string(authenticate_error(method_name)));

    NebulaLog::log("ReM",Log::ERROR,oss);

    xmlrpc_c::value_array arrayresult_error(arrayData);

    *retval = arrayresult_error;
    return;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
