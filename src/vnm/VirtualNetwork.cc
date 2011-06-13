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


#include "VirtualNetwork.h"
#include "VirtualNetworkPool.h"
#include "VirtualNetworkTemplate.h"

#include "NebulaLog.h"
#include "RangedLeases.h"
#include "FixedLeases.h"

#include "AuthManager.h"

/* ************************************************************************** */
/* Virtual Network :: Constructor/Destructor                                  */
/* ************************************************************************** */

VirtualNetwork::VirtualNetwork(int _uid,
                               int _gid,
                               VirtualNetworkTemplate *_vn_template):
                PoolObjectSQL(-1,"",_uid,_gid,table),
                bridge(""),
                type(UNINITIALIZED),
                leases(0)
{
    if (_vn_template != 0)
    {
        obj_template = _vn_template;
    }
    else
    {
        obj_template = new VirtualNetworkTemplate;
    }
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

VirtualNetwork::~VirtualNetwork()
{
    if (leases != 0)
    {
        delete leases;
    }

    if (obj_template != 0)
    {
        delete obj_template;
    }
}

/* ************************************************************************** */
/* Virtual Network :: Database Access Functions                               */
/* ************************************************************************** */

const char * VirtualNetwork::table        = "network_pool";

const char * VirtualNetwork::db_names     = "oid, name, body, uid, gid, public";

const char * VirtualNetwork::db_bootstrap = "CREATE TABLE IF NOT EXISTS"
    " network_pool (oid INTEGER PRIMARY KEY, name VARCHAR(128),"
    " body TEXT, uid INTEGER, gid INTEGER, public INTEGER, UNIQUE(name,uid))";

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualNetwork::select(SqlDB * db)
{
    int             rc;

    rc = PoolObjectSQL::select(db);

    if ( rc != 0 )
    {
        return rc;
    }

    return select_leases(db);
}

/* -------------------------------------------------------------------------- */

int VirtualNetwork::select(SqlDB * db, const string& name, int uid)
{
    int             rc;

    rc = PoolObjectSQL::select(db,name,uid);

    if ( rc != 0 )
    {
        return rc;
    }

    return select_leases(db);
}

/* -------------------------------------------------------------------------- */

int VirtualNetwork::select_leases(SqlDB * db)
{
    ostringstream   oss;
    ostringstream   ose;

    string          network_address;

    unsigned int default_size = VirtualNetworkPool::default_size();
    unsigned int mac_prefix   = VirtualNetworkPool::mac_prefix();

    //Get the leases
    if (type == RANGED)
    {
        string  nclass = "";
        int     size = 0;

        // retrieve specific information from the template
        get_template_attribute("NETWORK_ADDRESS",network_address);

        if (network_address.empty())
        {
            goto error_addr;
        }

        get_template_attribute("NETWORK_SIZE",nclass);

        if ( nclass == "B" || nclass == "b" )
        {
            size = 65534;
        }
        else if ( nclass == "C" || nclass == "c" )
        {
            size = 254;
        }
        else if (!nclass.empty()) //Assume it's a number
        {
            istringstream iss(nclass);
            iss >> size;
        }

        if (size == 0)
        {
            size = default_size;
        }

        leases = new RangedLeases(db,
                                  oid,
                                  size,
                                  mac_prefix,
                                  network_address);
    }
    else if(type == FIXED)
    {
        leases = new  FixedLeases(db,
                                  oid,
                                  mac_prefix);
    }
    else
    {
        goto error_type;
    }

    if (leases == 0)
    {
        goto error_leases;
    }

    return leases->select(db);


error_leases:
    ose << "Error getting Virtual Network leases nid: " << oid;
    goto error_common;

error_type:
    ose << "Wrong type of Virtual Network: " << type;
    goto error_common;

error_addr:
    ose << "Network address is not defined nid: " << oid;

error_common:
    NebulaLog::log("VNM", Log::ERROR, ose);
    return -1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualNetwork::insert(SqlDB * db, string& error_str)
{
    ostringstream   ose;
    int             rc;

    string          pub;
    string          s_type;

    unsigned int default_size = VirtualNetworkPool::default_size();
    unsigned int mac_prefix   = VirtualNetworkPool::mac_prefix();

    //--------------------------------------------------------------------------
    // VirtualNetwork Attributes from the template
    //--------------------------------------------------------------------------

    // ------------ TYPE ----------------------
    get_template_attribute("TYPE",s_type);

    transform(s_type.begin(),s_type.end(),s_type.begin(),(int(*)(int))toupper);

    if (s_type == "RANGED")
    {
        type = VirtualNetwork::RANGED;
    }
    else if ( s_type == "FIXED")
    {
        type = VirtualNetwork::FIXED;
    }
    else if ( s_type.empty() )
    {
        goto error_type_defined;
    }
    else
    {
        goto error_wrong_type;
    }

    // ------------ NAME ----------------------

    get_template_attribute("NAME",name);

    if (name.empty())
    {
        goto error_name;
    }

    // ------------ BRIDGE --------------------

    get_template_attribute("BRIDGE",bridge);

    if (bridge.empty())
    {
        goto error_bridge;
    }

    // ------------ PUBLIC --------------------

    get_template_attribute("PUBLIC", pub);

    transform (pub.begin(), pub.end(), pub.begin(), (int(*)(int))toupper);

    public_obj = (pub == "YES");

    obj_template->erase("PUBLIC");

    //--------------------------------------------------------------------------
    // Get the leases
    //--------------------------------------------------------------------------
    if (type == VirtualNetwork::RANGED)
    {
        string nclass = "";
        string naddr  = "";
        int    size   = 0;

        // retrieve specific information from template
        get_template_attribute("NETWORK_ADDRESS",naddr);

        if (naddr.empty())
        {
            goto error_addr;
        }

        get_template_attribute("NETWORK_SIZE",nclass);

        if ( nclass == "B" || nclass == "b"  )
        {
            size = 65534;
        }
        else if ( nclass == "C" || nclass == "c"  )
        {
            size = 254;
        }
        else if (!nclass.empty())//Assume its a number
        {
            istringstream iss(nclass);

            iss >> size;
        }

        if (size == 0)
        {
            SingleAttribute * attribute;
            ostringstream     oss;

            oss << default_size;

            attribute = new SingleAttribute("NETWORK_SIZE",oss.str());
            obj_template->set(attribute);

            size = default_size;
        }

        leases = new RangedLeases(db,
                                  oid,
                                  size,
                                  mac_prefix,
                                  naddr);
    }
    else // VirtualNetwork::FIXED
    {
        vector<const Attribute *>   vector_leases;

        get_template_attribute("LEASES",vector_leases);

        leases = new FixedLeases(db,
                                 oid,
                                 mac_prefix,
                                 vector_leases);
    }

    if (leases == 0)
    {
        goto error_null_leases;
    }

    //--------------------------------------------------------------------------
    // Insert the Virtual Network
    //--------------------------------------------------------------------------
    rc = insert_replace(db, false);

    if ( rc != 0 )
    {
        goto error_update;
    }

    return 0;

error_type_defined:
    ose << "No TYPE in template for Virtual Network.";
    goto error_common;

error_wrong_type:
    ose << "Wrong type \""<< s_type <<"\" in template for Virtual Network.";
    goto error_common;

error_name:
    ose << "No NAME in template for Virtual Network.";
    goto error_common;

error_bridge:
    ose << "No BRIDGE in template for Virtual Network.";
    goto error_common;

error_update:
    ose << "Can not update Virtual Network.";
    goto error_common;

error_addr:
    ose << "No NETWORK_ADDRESS in template for Virtual Network.";
    goto error_common;

error_null_leases:
    ose << "Error getting Virtual Network leases.";

error_common:
    error_str = ose.str();
    NebulaLog::log("VNM", Log::ERROR, ose);
    return -1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualNetwork::insert_replace(SqlDB *db, bool replace)
{
    ostringstream   oss;
    int             rc;


    string xml_body;

    char * sql_name;
    char * sql_xml;


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

    // Construct the SQL statement to Insert or Replace
    if(replace)
    {
        oss << "REPLACE";
    }
    else
    {
        oss << "INSERT";
    }

    oss << " INTO " << table << " (" << db_names << ") VALUES ("
        <<          oid         << ","
        << "'" <<   sql_name    << "',"
        << "'" <<   sql_xml     << "',"
        <<          uid         << ","
        <<          gid         << ","
        <<          public_obj << ")";

    rc = db->exec(oss);

    db->free_str(sql_name);
    db->free_str(sql_xml);

    return rc;


error_body:
    db->free_str(sql_name);
error_name:
    return -1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualNetwork::drop(SqlDB * db)
{
    int rc;

    rc = PoolObjectSQL::drop(db);

    if ( rc == 0 && leases != 0 )
    {
        rc += leases->drop(db);
    }

    return rc;
}

/* ************************************************************************** */
/* Virtual Network :: Misc                                                    */
/* ************************************************************************** */

string& VirtualNetwork::to_xml(string& xml) const
{
    return to_xml_extended(xml,false);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

string& VirtualNetwork::to_xml_extended(string& xml) const
{
    return to_xml_extended(xml,true);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

string& VirtualNetwork::to_xml_extended(string& xml, bool extended) const
{
    ostringstream os;

    string template_xml;
    string leases_xml;

    // Total leases is the number of used leases.
    int total_leases = 0;

    if (leases != 0)
    {
        total_leases = leases->n_used;
    }

    os <<
        "<VNET>" <<
            "<ID>"          << oid          << "</ID>"          <<
            "<UID>"         << uid          << "</UID>"         <<
            "<GID>"         << gid          << "</GID>"         <<
            "<NAME>"        << name         << "</NAME>"        <<
            "<TYPE>"        << type         << "</TYPE>"        <<
            "<BRIDGE>"      << bridge       << "</BRIDGE>"      <<
            "<PUBLIC>"      << public_obj  << "</PUBLIC>"      <<
            "<TOTAL_LEASES>"<< total_leases << "</TOTAL_LEASES>"<<
            obj_template->to_xml(template_xml);

    if (extended && leases != 0)
    {
        os << leases->to_xml(leases_xml);
    }

    os << "</VNET>";

    xml = os.str();

    return xml;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualNetwork::from_xml(const string &xml_str)
{
    vector<xmlNodePtr> content;

    int rc = 0;
    int int_type;

    // Initialize the internal XML object
    update_from_str(xml_str);

    // Get class base attributes
    rc += xpath(oid,        "/VNET/ID",         -1);
    rc += xpath(uid,        "/VNET/UID",        -1);
    rc += xpath(gid,        "/VNET/GID",        -1);
    rc += xpath(name,       "/VNET/NAME",       "not_found");
    rc += xpath(int_type,   "/VNET/TYPE",       -1);
    rc += xpath(bridge,     "/VNET/BRIDGE",     "not_found");
    rc += xpath(public_obj,"/VNET/PUBLIC",     0);

    type = static_cast<NetworkType>( int_type );

    // Get associated classes
    ObjectXML::get_nodes("/VNET/TEMPLATE", content);

    if( content.size() < 1 )
    {
        return -1;
    }

    // Virtual Network template
    rc += obj_template->from_xml_node( content[0] );

    if (rc != 0)
    {
        return -1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualNetwork::nic_attribute(VectorAttribute *nic, int vid)
{
    int rc;

    string  ip;
    string  mac;

    ostringstream  vnid;

    ip      = nic->vector_value("IP");
    vnid   << oid;

    //--------------------------------------------------------------------------
    //                       GET NETWORK LEASE
    //--------------------------------------------------------------------------

    if (ip.empty())
    {
        rc = leases->get(vid,ip,mac);
    }
    else
    {
        rc = leases->set(vid,ip,mac);
    }

    if ( rc != 0 )
    {
        return -1;
    }

    //--------------------------------------------------------------------------
    //                       NEW NIC ATTRIBUTES
    //--------------------------------------------------------------------------

    nic->replace("NETWORK"   ,name);
    nic->replace("NETWORK_ID",vnid.str());
    nic->replace("BRIDGE"    ,bridge);
    nic->replace("MAC"       ,mac);
    nic->replace("IP"        ,ip);

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualNetwork::add_leases(VirtualNetworkTemplate * leases_template,
                               string&                  error_msg)
{
    vector<const Attribute *> vector_leases;

    leases_template->get("LEASES", vector_leases);

    return leases->add_leases(vector_leases, error_msg);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualNetwork::remove_leases(VirtualNetworkTemplate * leases_template,
                                  string&                  error_msg)
{
    vector<const Attribute *> vector_leases;

    leases_template->get("LEASES", vector_leases);

    return leases->remove_leases(vector_leases, error_msg);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
