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

#ifndef CLUSTER_H_
#define CLUSTER_H_

#include "PoolSQL.h"
#include "Host.h"
#include "ObjectCollection.h"

using namespace std;

/**
 *  The Cluster class.
 */
class Cluster : public PoolObjectSQL, public ObjectCollection
{
public:

    /**
     *  Function to write a Cluster on an output stream
     */
     friend ostream& operator<<(ostream& os, Cluster& cluster);

    /**
     * Function to print the Cluster object into a string in XML format
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
     *  Some PoolObjectSQL sub-classes are also a sub-class of ObjectCollection.
     *
     *    @return a pointer to the object.
     */
    ObjectCollection * get_collection()
    {
        return this;
    };

private:

    // -------------------------------------------------------------------------
    // Friends
    // -------------------------------------------------------------------------

    friend class ClusterPool;

    // *************************************************************************
    // Constructor
    // *************************************************************************

    Cluster(int id, const string& name):
        PoolObjectSQL(id,name,-1,-1,table),
        ObjectCollection("HOSTS"){};

    virtual ~Cluster(){};

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
     *  Bootstraps the database table(s) associated to the Cluster
     */
    static void bootstrap(SqlDB * db)
    {
        ostringstream oss_cluster(Cluster::db_bootstrap);

        db->exec(oss_cluster);
    };

    /**
     *  Writes the Cluster in the database.
     *    @param db pointer to the db
     *    @return 0 on success
     */
    int insert(SqlDB *db, string& error_str);

    /**
     *  Writes/updates the Cluster's data fields in the database.
     *    @param db pointer to the db
     *    @return 0 on success
     */
    int update(SqlDB *db);


    // *************************************************************************
    // Host IDs set
    // *************************************************************************

    /**
     *  Moves all hosts in this cluster to the default one. Must be called
     *  before the cluster is dropped.
     */
    int set_default_cluster();
};

#endif /*CLUSTER_H_*/
