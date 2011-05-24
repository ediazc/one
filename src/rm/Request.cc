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

#include "Request.h"
#include "Nebula.h"


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void Request::execute(
        xmlrpc_c::paramList const& _paramList,
        xmlrpc_c::value *   const  _retval)
{
    int uid;
    int gid;

    retval  = _retval;
    session = xmlrpc_c::value_string (_paramList.getString(0));

    Nebula& nd = Nebula::instance();
    UserPool* upool = nd.get_upool();

    NebulaLog::log("ReM",Log::DEBUG, method_name + " method invoked");

if (true)   // if ( upool->authenticate(uid, gid) == false )
    {
        failure_response(RequestManager::AUTHENTICATION,
                         authenticate_error());
    }
    else
    {
        request_execute(uid, gid, _paramList);    
    }
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void Request::failure_response(RequestManager::ErrorCode ec, 
                               const string& str_val)
{    
    vector<xmlrpc_c::value> arrayData;

    arrayData.push_back(xmlrpc_c::value_boolean(false));
    arrayData.push_back(xmlrpc_c::value_string(str_val));
    arrayData.push_back(xmlrpc_c::value_int(ec));

    xmlrpc_c::value_array arrayresult(arrayData);

    *retval = arrayresult;

    NebulaLog::log("ReM",Log::ERROR,str_val);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void Request::success_response(int id)
{    
    vector<xmlrpc_c::value> arrayData;

    arrayData.push_back(xmlrpc_c::value_boolean(true));
    arrayData.push_back(xmlrpc_c::value_int(id));

    xmlrpc_c::value_array arrayresult(arrayData);

    *retval = arrayresult;
}

/* -------------------------------------------------------------------------- */

void Request::success_response(const string& val)
{    
    vector<xmlrpc_c::value> arrayData;

    arrayData.push_back(xmlrpc_c::value_boolean(true));
    arrayData.push_back(xmlrpc_c::value_string(val));

    xmlrpc_c::value_array arrayresult(arrayData);

    *retval = arrayresult;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

string Request::authorization_error (const string &action,
                                     const string &object,
                                     int   uid,
                                     int   id)
{
    ostringstream oss;

    oss << "[" << method_name << "]" << " User [" << uid << "] not authorized"
        << " to perform " << action << " on " << object;


    if ( id != -1 )
    {
        oss << " [" << id << "].";
    }
    else
    {
        oss << " Pool";
    }

    return oss.str();
}

/* -------------------------------------------------------------------------- */

string Request::authenticate_error()
{
    ostringstream oss;

    oss << "[" << method_name << "]" << " User couldn't be authenticated," <<
           " aborting call.";

    return oss.str();
}

/* -------------------------------------------------------------------------- */

string Request::get_error (const string &object,
                           int id)
{
    ostringstream oss;

    oss << "[" << method_name << "]" << " Error getting " <<
           object;

   if ( id != -1 )
   {
       oss << " [" << id << "].";
   }
   else
   {
      oss << " Pool.";
   }

   return oss.str();
}
/* -------------------------------------------------------------------------- */

string Request::action_error (const string &action,
                                const string &object,
                                int id,
                                int rc)
{
    ostringstream oss;

    oss << "[" << method_name << "]" << " Error trying to " << action << " "
        << object;

    switch(id)
    {
        case -2:
            break;
        case -1:
            oss << "Pool.";
            break;
        default:
            oss << " [" << id << "].";
            break;
    }

    if ( rc != 0 )
    {
        oss << " Returned error code [" << rc << "].";
    }

    return oss.str();
}
