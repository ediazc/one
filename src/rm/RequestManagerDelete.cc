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

#include "RequestManagerDelete.h"

using namespace std;

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

void RequestManagerDelete::request_execute(xmlrpc_c::paramList const& paramList)
{
    int             oid = xmlrpc_c::value_int(paramList.getInt(1));
    PoolObjectSQL * object;
    string          error_msg;

    if ( basic_authorization(oid) == false )
    {
        return;
    }

    object = pool->get(oid,true);

    if ( object == 0 )                             
    {                                            
        failure_response(NO_EXISTS, get_error(object_name(auth_object),oid));
        return;
    }    

    int rc = drop(oid, object, error_msg);

    if ( rc != 0 )
    {
        failure_response(INTERNAL,
            request_error("Can not delete "+object_name(auth_object),error_msg));
        return;
    }

    success_response(oid);

    return;
}

/* ------------------------------------------------------------------------- */

int ImageDelete::drop(int oid, PoolObjectSQL * object, string& error_msg)
{
    Nebula&         nd     = Nebula::instance();
    ImageManager *  imagem = nd.get_imagem();

    object->unlock();
    int rc = imagem->delete_image(oid);

    return rc;
}

/* ------------------------------------------------------------------------- */

int UserDelete::drop(int oid, PoolObjectSQL * object, string& error_msg)
{
    set<int>        group_set;

    User * user = static_cast<User *>(object);
    group_set   = user->get_groups();

    int rc = pool->drop(object, error_msg);

    object->unlock();

    if ( rc == 0 )
    {
        Nebula&     nd      = Nebula::instance();
        GroupPool * gpool   = nd.get_gpool();

        Group *     group;

        set<int>::iterator  it;

        for ( it = group_set.begin(); it != group_set.end(); it++ )
        {
            group = gpool->get(*it, true);

            if( group == 0 )
            {
                continue;
            }

            group->del_user(oid);
            gpool->update(group);

            group->unlock();
        }
    }

    return rc;
}
