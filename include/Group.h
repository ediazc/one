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
/* -------------------------------------------------------------------------*/

#ifndef GROUP_H_
#define GROUP_H_

#include "PoolSQL.h"
#include "ObjectCollection.h"
#include "User.h"

using namespace std;

/**
 *  The Group class.
 */
class Group : public PoolObjectSQL, public ObjectCollection
{
public:

    /**
     * Function to print the Group object into a string in XML format
     *  @param xml the resulting XML string
     *  @return a reference to the generated string
     */
    string& to_xml(string& xml) const;

    /**
     *  Rebuilds the object from an xml formatted string
     *    @param xml_str The xml-formatted string
     *
     *    @return 0 on success, -1 otherwise
     */
    int from_xml(const string &xml_str);

    /**
<<<<<<< HEAD
     *  Adds this object's ID to the set. The object MUST be a User, locked.
     *  The group's ID is added to the User's set.
     *    @param object The new object
     *    @param error_str Error reason, if any
     *
     *    @return 0 on success, -1 if the ID was already in the set
     */
    int add_collection_id(PoolObjectSQL* object, string& error_str);

    /**
     *  Deletes this object's ID from the set. The object MUST be a User,
     *  locked. The group's ID is deleted form the User's set.
     *    @param object The object
     *    @param error_str Error reason, if any
     *
     *    @return 0 on success, -1 if the ID was not in the set
     */
    int del_collection_id(PoolObjectSQL* object, string& error_str);

    /**
     *  Some PoolObjectSQL sub-classes are also a sub-class of ObjectCollection.
     *
     *    @return a pointer to the object.
     */
    ObjectCollection * get_collection()
    {
        return this;
    };
=======
     *  Adds this user's ID to the set. 
     *    @param id of the user to be added to the group
     *    @return 0 on success
     */
    int add_user(int id)
    {
        return add_collection_id(id);
    }

    /**
     *  Deletes this users's ID from the set.
     *    @param id of the user to be deleted from the group
     *    @return 0 on success
     */
    int del_user(int id)
    {
        return del_collection_id(id);
    }
>>>>>>> master

private:

    // -------------------------------------------------------------------------
    // Friends
    // -------------------------------------------------------------------------

    friend class GroupPool;

    // *************************************************************************
    // Constructor
    // *************************************************************************

    Group(int id, const string& name):
        PoolObjectSQL(id,name,-1,-1,table),
        ObjectCollection("USERS"){};

    virtual ~Group(){};

    // *************************************************************************
    // DataBase implementation (Private)
    // *************************************************************************

    static const char * db_names;

    static const char * db_bootstrap;

    static const char * table;

    /**
     *  Execute an INSERT or REPLACE Sql query.
     *    @param db The SQL DB
     *    @param replace Execute an INSERT or a REPLACE
     *    @return 0 one success
    */
    int insert_replace(SqlDB *db, bool replace);

    /**
     *  Bootstraps the database table(s) associated to the Group
     */
    static void bootstrap(SqlDB * db)
    {
        ostringstream oss(Group::db_bootstrap);

        db->exec(oss);
    };

    /**
     *  Writes the Group in the database.
     *    @param db pointer to the db
     *    @return 0 on success
     */
    int insert(SqlDB *db, string& error_str)
    {
        int rc;

        rc = insert_replace(db, false);

        if ( rc != 0 )
        {
            error_str = "Error inserting Group in DB.";
        }

        return rc;
    }

    /**
     *  Writes/updates the Group's data fields in the database.
     *    @param db pointer to the db
     *    @return 0 on success
     */
    int update(SqlDB *db)
    {
        return insert_replace(db, true);
    }
<<<<<<< HEAD

    // *************************************************************************
    // ID Set management
    // *************************************************************************

    int add_del_collection_id(User* object, bool add, string& error_str);
=======
>>>>>>> master
};

#endif /*GROUP_H_*/
