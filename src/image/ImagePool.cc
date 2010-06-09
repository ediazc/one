/* -------------------------------------------------------------------------- */
/* Copyright 2002-2010, OpenNebula Project Leads (OpenNebula.org)             */
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

/* ************************************************************************** */
/* Image Pool                                                                 */
/* ************************************************************************** */

#include "ImagePool.h"
#include <openssl/evp.h>
#include <iomanip>


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int ImagePool::init_cb(void *nil, int num, char **values, char **names)
{
    if ( num == 0 || values == 0 || values[0] == 0 )
    {
        return -1;
    }

    image_names.insert(make_pair(values[1],atoi(values[0])));

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

ImagePool::ImagePool(   SqlDB * db,
                        const string&   _source_prefix,
                        const string&   _default_type,
                        const string&   _default_dev_prefix):

                        PoolSQL(db,Image::table),
                        source_prefix(_source_prefix),
                        default_dev_prefix(_default_dev_prefix)
{
    ostringstream   sql;
    int             rc;

    // Set default type
    if (_default_type != "OS"       &&
        _default_type != "CDROM"    &&
        _default_type != "DATABLOCK" )
    {
        NebulaLog::log("IMG", Log::ERROR,
                 "Bad default for image type, setting OS");
        default_type = "OS";
    }
    else
    {
        default_type = _default_type;
    }


    // Read from the DB the existing images, and build the ID:Name map
    set_callback(static_cast<Callbackable::Callback>(&ImagePool::init_cb));

    sql  << "SELECT oid, name FROM " <<  Image::table;

    rc = db->exec(sql, this);

    if ( rc != 0 )
    {
        NebulaLog::log("IMG", Log::ERROR,
                 "Could not load the existing images from the DB.");
    }
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int ImagePool::allocate (
        int            uid,
        const  string& stemplate,
        int *          oid)
{
        Image * img;
        string  name           = "";
        string  source         = "";
        string  type           = "";
        string  public_attr    = "";
        string  original_path  = "";
        string  dev_prefix     = "";

        ostringstream          tmp_hashstream;
        ostringstream          tmp_sourcestream;

        char *  error_msg;
        int     rc;


        // ---------------------------------------------------------------------
        // Build a new Image object
        // ---------------------------------------------------------------------
        img = new Image(uid);

        // ---------------------------------------------------------------------
        // Parse template 
        // ---------------------------------------------------------------------
        rc = img->image_template.parse(stemplate, &error_msg);

        if ( rc != 0 )
        {
            goto error_parse;
        }
        
        // ---------------------------------------------------------------------
        // Check default image attributes
        // ---------------------------------------------------------------------               
        img->get_template_attribute("NAME", name);

        if ( name.empty() == true )
        {
            goto error_name;
        }

        img->image_template.erase("NAME");


        img->get_template_attribute("TYPE", type);

        if ( type.empty() == true )
        {
            type = default_type;
        }
        else
        {
            img->image_template.erase("TYPE");
        }

        img->get_template_attribute("PUBLIC", public_attr);

        if ( public_attr.empty() == true )
        {
            public_attr = "NO";
        }
        else
        {
            img->image_template.erase("PUBLIC");
        }

        img->get_template_attribute("ORIGINAL_PATH", original_path);
        
        if  ( (type == "OS" || type == "CDROM") &&
               original_path.empty() == true      )
        {
            goto error_original_path;
        }

        // DEV_PREFIX template attribute must exist for every image, if it
        // isn't present it will be set to the default value.

        img->get_template_attribute("DEV_PREFIX", dev_prefix);

        if( dev_prefix.empty() )
        {
            SingleAttribute * dev_att = 
                        new SingleAttribute("DEV_PREFIX", default_dev_prefix);

            img->image_template.set(dev_att);
        }


        img->running_vms = 0;


        // Generate path to store the image
        tmp_hashstream << uid << ":" << name;

        tmp_sourcestream << source_prefix << "/";
        tmp_sourcestream << sha1_digest(tmp_hashstream.str());

        img->name        = name;
        img->source      = tmp_sourcestream.str();
        img->public_img  = public_attr;

        if (img->set_type(type) != 0)
        {
            goto error_type;
        }

        
        // ---------------------------------------------------------------------
        // Insert the Object in the pool
        // ---------------------------------------------------------------------

        *oid = PoolSQL::allocate(img);

        if ( *oid == -1 )
        {
            return -1;
        }

        // Add the image name to the map of image_names
        image_names.insert(make_pair(name, *oid));

        return *oid;

error_name:
    NebulaLog::log("IMG", Log::ERROR, "NAME not present in image template");
    goto error_common;
error_type:
    NebulaLog::log("IMG", Log::ERROR, "Incorrect TYPE in image template");
    goto error_common;
error_original_path:
    NebulaLog::log("IMG", Log::ERROR, 
    "ORIGINAL_PATH compulsory and not present in image template of this type.");
    goto error_common;
error_common:
    delete img;
    *oid = -1;
    return -1;
    
error_parse:
    ostringstream oss;
    oss << "ImagePool template parse error: " << error_msg;
    NebulaLog::log("IMG", Log::ERROR, oss);
    free(error_msg);
    delete img;
    *oid = -2;
    return -2;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int ImagePool::dump_cb(void * _oss, int num, char **values, char **names)
{
    ostringstream * oss;

    oss = static_cast<ostringstream *>(_oss);

    return Image::dump(*oss, num, values, names);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int ImagePool::dump(ostringstream& oss, const string& where)
{
    int             rc;
    ostringstream   cmd;

    oss << "<IMAGE_POOL>";

    set_callback(static_cast<Callbackable::Callback>(&ImagePool::dump_cb),
                  static_cast<void *>(&oss));

    cmd << "SELECT * FROM " << Image::table;

    if ( !where.empty() )
    {
        cmd << " WHERE " << where;
    }

    rc = db->exec(cmd, this);

    oss << "</IMAGE_POOL>";

    return rc;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

string ImagePool::sha1_digest(const string& pass)
{
    EVP_MD_CTX     mdctx;
    unsigned char  md_value[EVP_MAX_MD_SIZE];
    unsigned int   md_len;
    ostringstream  oss;

    EVP_MD_CTX_init(&mdctx);
    EVP_DigestInit_ex(&mdctx, EVP_sha1(), NULL);

    EVP_DigestUpdate(&mdctx, pass.c_str(), pass.length());

    EVP_DigestFinal_ex(&mdctx,md_value,&md_len);
    EVP_MD_CTX_cleanup(&mdctx);

    for(unsigned int i = 0; i<md_len; i++)
    {
        oss << setfill('0') << setw(2) << hex << nouppercase
            << (unsigned short) md_value[i];
    }

    return oss.str();
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

