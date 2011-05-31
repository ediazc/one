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

void RequestManager::ClusterRemove::execute(
    xmlrpc_c::paramList const& paramList,
    xmlrpc_c::value *   const  retval)
{
    string  session;

    int     hid, host_gid;
    int     clid;
    int     rc;
    
    const string        method_name = "ClusterRemove";

    Host *      host;
    Cluster *   cluster;

    string          error_str;
    ostringstream   oss;

    /*   -- RPC specific vars --  */
    vector<xmlrpc_c::value> arrayData;
    xmlrpc_c::value_array * arrayresult;

    NebulaLog::log("ReM",Log::DEBUG,"ClusterRemove method invoked");

    // Get the parameters
    session      = xmlrpc_c::value_string(paramList.getString(0));
    hid          = xmlrpc_c::value_int   (paramList.getInt(1));
    clid         = ClusterPool::DEFAULT_CLUSTER_ID;

    //Authenticate the user
    rc = ClusterRemove::upool->authenticate(session);

    if ( rc == -1 )
    {
        goto error_authenticate;
    }

     //Authorize the operation
    if ( rc != 0 ) // rc == 0 means oneadmin
    {
        AuthRequest ar(rc);
        
        ar.add_auth(AuthRequest::HOST,hid,AuthRequest::MANAGE,0,false);
        ar.add_auth(AuthRequest::CLUSTER,clid,AuthRequest::USE,0,false);

        if (UserPool::authorize(ar) == -1)
        {
            goto error_authorize;
        }
    }

    // Check if cluster exists
    cluster = ClusterRemove::cpool->get(clid,true);

    if ( cluster == 0 )
    {
        goto error_cluster_get;
    }

    // Check if host exists
    host = ClusterRemove::hpool->get(hid,true);

    if ( host == 0 )
    {
        goto error_host_get;
    }

    // Get current host cluster
    host_gid = host->get_gid();

    // Set cluster
    rc = host->set_gid(cluster->get_oid());

    // Add host ID to cluster
    if( rc == 0 )
    {
        static_cast<ObjectCollection*>(cluster)->add_collection_id(
                                                            host, error_str);
    }

    if ( rc != 0 )
    {
        goto error_cluster_add;
    }

    // Update the DB
    ClusterRemove::hpool->update(host);
    ClusterRemove::cpool->update(cluster);

    cluster->unlock();

    // TODO: This lock-unlock order may cause deadlocks

    // Now get the old cluster, and remove the Host Id from it
    cluster = ClusterRemove::cpool->get(host_gid, true);

    if( cluster != 0 )
    {
        cluster->del_collection_id(host, error_str);
        ClusterRemove::cpool->update(cluster);

        cluster->unlock();
    }

    host->unlock();


    // All nice, return success to the client
    arrayData.push_back(xmlrpc_c::value_boolean(true)); // SUCCESS

    // Copy arrayresult into retval mem space
    arrayresult = new xmlrpc_c::value_array(arrayData);
    *retval = *arrayresult;

    delete arrayresult; // and get rid of the original

    return;

error_authenticate:
    oss.str(authenticate_error(method_name));
    goto error_common;

error_authorize:
    oss.str(authorization_error(method_name, "USE", "CLUSTER", rc, clid));
    goto error_common;

error_host_get:
    cluster->unlock();
    oss.str(get_error(method_name, "HOST", hid));
    goto error_common;

error_cluster_get:
    oss.str(get_error(method_name, "CLUSTER", clid));
    goto error_common;

error_cluster_add:
    host->unlock();
    cluster->unlock();
    oss << action_error(method_name, "USE", "CLUSTER", clid, rc)
        << " Reason: " << error_str;

    goto error_common;

error_common:

    arrayData.push_back(xmlrpc_c::value_boolean(false)); // FAILURE
    arrayData.push_back(xmlrpc_c::value_string(oss.str()));

    NebulaLog::log("ReM",Log::ERROR,oss);

    xmlrpc_c::value_array arrayresult_error(arrayData);

    *retval = arrayresult_error;

    return;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
