/* ------------------------------------------------------------------------ */
/* Copyright 2002-2010, OpenNebula Project Leads (OpenNebula.org)           */
/*                                                                          */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may  */
/* not use this file except in compliance with the License. You may obtain  */
/* a copy of the License at                                                 */
/*                                                                          */
/* http://www.apache.org/licenses/LICENSE-2.0                               */
/*                                                                          */
/* Unless required by applicable law or agreed to in writing, software      */
/* distributed under the License is distributed on an "AS IS" BASIS,        */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. */
/* See the License for the specific language governing permissions and      */
/* limitations under the License.                                           */
/* ------------------------------------------------------------------------ */

#include <limits.h>
#include <string.h>

#include <iostream>
#include <sstream>

#include "Group.h"

const char * Group::table = "group_pool";

const char * Group::db_names = "oid, name, body";

const char * Group::db_bootstrap = "CREATE TABLE IF NOT EXISTS group_pool ("
    "oid INTEGER PRIMARY KEY, name VARCHAR(256), body TEXT, "
    "UNIQUE(name))";

/* ************************************************************************ */
/* Group :: Database Access Functions                                       */
/* ************************************************************************ */

int Group::insert_replace(SqlDB *db, bool replace)
{
    ostringstream   oss;

    int    rc;
    string xml_body;

    char * sql_name;
    char * sql_xml;

    // Update the Group

    sql_name = db->escape_str(name.c_str());

    if ( sql_name == 0 )
    {
        goto error_name;
    }

    sql_xml = db->escape_str(to_xml(xml_body).c_str());

    if ( sql_xml == 0 )
    {
        goto error_body;
    }

    if ( replace )
    {
        oss << "REPLACE";
    }
    else
    {
        oss << "INSERT";
    }

    // Construct the SQL statement to Insert or Replace

    oss <<" INTO "<<table <<" ("<< db_names <<") VALUES ("
        <<          oid                 << ","
        << "'" <<   sql_name            << "',"
        << "'" <<   sql_xml             << "')";

    rc = db->exec(oss);

    db->free_str(sql_name);
    db->free_str(sql_xml);

    return rc;

error_body:
    db->free_str(sql_name);
error_name:
    return -1;
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

string& Group::to_xml(string& xml) const
{
    ostringstream   oss;
    string          collection_xml;

    ObjectCollection::to_xml(collection_xml);

    oss <<
    "<GROUP>"    <<
        "<ID>"   << oid  << "</ID>"   <<
        "<NAME>" << name << "</NAME>" <<
        collection_xml <<
    "</GROUP>";

    xml = oss.str();

    return xml;
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

int Group::from_xml(const string& xml)
{
    int rc = 0;
    vector<xmlNodePtr> content;

    // Initialize the internal XML object
    update_from_str(xml);

    // Get class base attributes
    rc += xpath(oid, "/GROUP/ID",   -1);
    rc += xpath(name,"/GROUP/NAME", "not_found");

    // Get associated classes
    ObjectXML::get_nodes("/GROUP/USERS", content);

    if( content.size() < 1 )
    {
        return -1;
    }

    // Set of IDs
    rc += ObjectCollection::from_xml_node(content[0]);

    if (rc != 0)
    {
        return -1;
    }

    return 0;
}

<<<<<<< HEAD
/* ************************************************************************ */
/* Group :: User ID Set                                                     */
/* ************************************************************************ */

int Group::add_collection_id(PoolObjectSQL* object, string& error_str)
{
    // TODO: make a dynamic cast and check if object is indeed a User ?
    return add_del_collection_id( static_cast<User*>(object), true, error_str);
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

int Group::del_collection_id(PoolObjectSQL* object, string& error_str)
{
    // TODO: make a dynamic cast and check if object is indeed a User ?
    return add_del_collection_id( static_cast<User*>(object), false, error_str);
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

int Group::add_del_collection_id(User* object, bool add, string& error_str)
{
    int rc = 0;
    ObjectCollection * object_collection = 0;

    // Get object id
    int object_id = object->get_oid();

    // Add/Remove object to the group
    if(add)
    {
        rc = ObjectCollection::add_collection_id(object_id, error_str);
    }
    else
    {
        rc = ObjectCollection::del_collection_id(object_id, error_str);
    }

    if( rc != 0 )
    {
        return -1;
    }

    // Users can be in more than one group, it has to store the
    // reverse relation
    object_collection = static_cast<ObjectCollection*>(object);

    if(add)
    {
        rc = object_collection->add_collection_id( this, error_str );
        if( rc != 0 )
        {
            string tmp_error;
            ObjectCollection::del_collection_id(object_id, tmp_error);
        }
    }
    else
    {
        rc = object_collection->del_collection_id( this, error_str );
        if( rc != 0 )
        {
            string tmp_error;
            ObjectCollection::add_collection_id(object_id, tmp_error);
        }
    }

    return rc;
}

=======
>>>>>>> master
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

