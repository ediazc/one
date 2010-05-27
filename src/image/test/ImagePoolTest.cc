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

#include <string>
#include <iostream>
#include <stdlib.h>

#include "ImagePool.h"
#include "PoolTest.h"

using namespace std;

const int uids[] = {122, 262, 127};

const string names[] = {"Image one", "Second Image", "The third image"};

const string templates[] =
{
    "NAME          = \"Image one\"\n"
    "ORIGINAL_PATH = /tmp/image_test\n"
    "DESCRIPTION   = \"This is a very long description of an image, and to achieve the longness I will copy this over. This is a very long description of an image, and to achieve the longness I will copy this over. And over. This is a very long description of an image, and to achieve the longness I will copy this over. And over. This is a very long description of an image, and to achieve the longness I will copy this over. And over.This is a very long description of an image, and to achieve the longness I will copy this over.\"\n",

    "NAME          = \"Second Image\"\n"
    "ORIGINAL_PATH = /tmp/image_second_test\n"
    "DESCRIPTION   = \"This is a rather short description.\"\n",

    "NAME          = \"The third image\"\n"
    "ORIGINAL_PATH = /tmp/image_test\n"
    "# DESCRIPTION   = \"An image description\"\n"
    "BUS           = SCSI\n"
    "PROFILE       = STUDENT\n"
};


const string xmls[] =
{
    "<IMAGE><ID>0</ID><UID>122</UID><NAME>Image one</NAME><TYPE>0</TYPE><REGTIME>0000000000</REGTIME><SOURCE>source_prefix/b734e8645d3eba91315e851662adf021ee488e34</SOURCE><STATE>0</STATE><RUNNING_VMS>0</RUNNING_VMS><TEMPLATE><DESCRIPTION><![CDATA[This is a very long description of an image, and to achieve the longness I will copy this over. This is a very long description of an image, and to achieve the longness I will copy this over. And over. This is a very long description of an image, and to achieve the longness I will copy this over. And over. This is a very long description of an image, and to achieve the longness I will copy this over. And over.This is a very long description of an image, and to achieve the longness I will copy this over.]]></DESCRIPTION><ORIGINAL_PATH><![CDATA[/tmp/image_test]]></ORIGINAL_PATH></TEMPLATE></IMAGE>",

    "<IMAGE><ID>1</ID><UID>262</UID><NAME>Second Image</NAME><TYPE>0</TYPE><REGTIME>0000000000</REGTIME><SOURCE>source_prefix/24939d280a0e29c9eb00bbb7c5e997ba5c4b8667</SOURCE><STATE>0</STATE><RUNNING_VMS>0</RUNNING_VMS><TEMPLATE><DESCRIPTION><![CDATA[This is a rather short description.]]></DESCRIPTION><ORIGINAL_PATH><![CDATA[/tmp/image_second_test]]></ORIGINAL_PATH></TEMPLATE></IMAGE>",

    "<IMAGE><ID>0</ID><UID>127</UID><NAME>The third image</NAME><TYPE>0</TYPE><REGTIME>0000000000</REGTIME><SOURCE>source_prefix/b690e963a810b60538b70784697b5183807f84ff</SOURCE><STATE>0</STATE><RUNNING_VMS>0</RUNNING_VMS><TEMPLATE><BUS><![CDATA[SCSI]]></BUS><ORIGINAL_PATH><![CDATA[/tmp/image_test]]></ORIGINAL_PATH><PROFILE><![CDATA[STUDENT]]></PROFILE></TEMPLATE></IMAGE>"
};


// This xml dump result has the STIMEs modified to 0000000000
const string xml_dump =
    "<IMAGE_POOL><IMAGE><ID>0</ID><UID>122</UID><NAME>Image one</NAME><TYPE>0</TYPE><REGTIME>0000000000</REGTIME><SOURCE>source_prefix/b734e8645d3eba91315e851662adf021ee488e34</SOURCE><STATE>0</STATE><RUNNING_VMS>0</RUNNING_VMS></IMAGE><IMAGE><ID>1</ID><UID>262</UID><NAME>Second Image</NAME><TYPE>0</TYPE><REGTIME>0000000000</REGTIME><SOURCE>source_prefix/24939d280a0e29c9eb00bbb7c5e997ba5c4b8667</SOURCE><STATE>0</STATE><RUNNING_VMS>0</RUNNING_VMS></IMAGE><IMAGE><ID>2</ID><UID>127</UID><NAME>The third image</NAME><TYPE>0</TYPE><REGTIME>0000000000</REGTIME><SOURCE>source_prefix/b690e963a810b60538b70784697b5183807f84ff</SOURCE><STATE>0</STATE><RUNNING_VMS>0</RUNNING_VMS></IMAGE></IMAGE_POOL>";

const string xml_dump_where =
    "<IMAGE_POOL><IMAGE><ID>1</ID><UID>262</UID><NAME>Second Image</NAME><TYPE>0</TYPE><REGTIME>0000000000</REGTIME><SOURCE>source_prefix/24939d280a0e29c9eb00bbb7c5e997ba5c4b8667</SOURCE><STATE>0</STATE><RUNNING_VMS>0</RUNNING_VMS></IMAGE><IMAGE><ID>2</ID><UID>127</UID><NAME>The third image</NAME><TYPE>0</TYPE><REGTIME>0000000000</REGTIME><SOURCE>source_prefix/b690e963a810b60538b70784697b5183807f84ff</SOURCE><STATE>0</STATE><RUNNING_VMS>0</RUNNING_VMS></IMAGE></IMAGE_POOL>";

const string replacement = "0000000000";


/* ************************************************************************* */
/* ************************************************************************* */

class ImagePoolTest : public PoolTest
{
    CPPUNIT_TEST_SUITE (ImagePoolTest);

    ALL_POOLTEST_CPPUNIT_TESTS();

    CPPUNIT_TEST ( update );
    CPPUNIT_TEST ( get_using_name );
    CPPUNIT_TEST ( wrong_get_name );
    CPPUNIT_TEST ( duplicates );
    CPPUNIT_TEST ( extra_attributes );
    CPPUNIT_TEST ( dump );
    CPPUNIT_TEST ( dump_where );

    CPPUNIT_TEST_SUITE_END ();

protected:

    void bootstrap(SqlDB* db)
    {
        ImagePool::bootstrap(db);
    };

    PoolSQL* create_pool(SqlDB* db)
    {
        return new ImagePool(db, "source_prefix", "OS", "IDE");
    };

    int allocate(int index)
    {
        int oid;
        return ((ImagePool*)pool)->allocate(uids[index],
                                            templates[index],
                                            &oid);

    };

    void check(int index, PoolObjectSQL* obj)
    {
        CPPUNIT_ASSERT( obj != 0 );

        string xml_str = "";

        // Get the xml and replace the REGTIME to 0, so we can compare
        // it.
        ((Image*)obj)->to_xml(xml_str);
        xml_str.replace( xml_str.find("<REGTIME>")+9, 10, replacement);

        CPPUNIT_ASSERT( ((Image*)obj)->get_name() == names[index] );
        CPPUNIT_ASSERT( xml_str == xmls[index]);
    };

public:
    ImagePoolTest(){};

    ~ImagePoolTest(){};


    /* ********************************************************************* */


    void update()
    {
        string      description_name    = "DESCRIPTION";
        string      description_val     = "";
        string      new_description     = "A new description";

        string      attr_name       = "NEW_ATTRIBUTE";
        string      attr_val        = "";
        string      new_attr_value  = "New value";

        string      no_value        = "Some random value";

        ImagePool * ip;
        Image *     img;
        int         oid_1;

        ip = static_cast<ImagePool *>(pool);
        oid_1 = allocate(0);

        img = ip->get(oid_1, true);
        CPPUNIT_ASSERT( img != 0 );

        // Image object should be cached. Let's change some template attributes
        img->update_template_attribute(db, description_name, new_description);
        img->update_template_attribute(db, attr_name,        new_attr_value);
        img->remove_template_attribute(db, "ORIGINAL_PATH");

        pool->update(img);

        img->unlock();

        img = ip->get(oid_1,false);
        CPPUNIT_ASSERT( img != 0 );


        img->get_template_attribute("DESCRIPTION",   description_val);
        img->get_template_attribute("NEW_ATTRIBUTE", attr_val);
        img->get_template_attribute("ORIGINAL_PATH", no_value);

        CPPUNIT_ASSERT( description_val == new_description );
        CPPUNIT_ASSERT( attr_val        == new_attr_value );
        CPPUNIT_ASSERT( no_value        == "" );

        //Now force access to DB

        pool->clean();
        img = ip->get(oid_1,false);

        CPPUNIT_ASSERT( img != 0 );

        description_val = "";
        attr_val        = "";
        no_value        = "Random value";
        img->get_template_attribute("DESCRIPTION",   description_val);
        img->get_template_attribute("NEW_ATTRIBUTE", attr_val);
        img->get_template_attribute("ORIGINAL_PATH", no_value);

        CPPUNIT_ASSERT( description_val == new_description );
        CPPUNIT_ASSERT( attr_val        == new_attr_value );
        CPPUNIT_ASSERT( no_value        == "" );
    };


    void get_using_name()
    {
        int oid_0, oid_1;
        ImagePool * imp = static_cast<ImagePool *>(pool);

        // Allocate two objects
        oid_0 = allocate(0);
        oid_1 = allocate(1);

        // ---------------------------------
        // Get first object and check its integrity
        obj = pool->get(oid_0, false);
        CPPUNIT_ASSERT( obj != 0 );
        check(0, obj);

        // Get using its name
        obj = imp->get(names[1], true);
        CPPUNIT_ASSERT( obj != 0 );
        obj->unlock();

        check(1, obj);


        // ---------------------------------
        // Clean the cache, forcing the pool to read the objects from the DB
        pool->clean();

        // Get first object and check its integrity
        obj = imp->get(names[0], false);
        check(0, obj);

        // Get using its name
        obj = imp->get(oid_1, false);
        check(1, obj);
    };


    void wrong_get_name()
    {
        ImagePool * imp = static_cast<ImagePool *>(pool);

        // The pool is empty
        // Non existing name
        obj = imp->get("Wrong name", true);
        CPPUNIT_ASSERT( obj == 0 );

        // Allocate an object
        allocate(0);

        // Ask again for a non-existing name
        obj = imp->get("Non existing name", true);
        CPPUNIT_ASSERT( obj == 0 );
    }


    void duplicates()
    {
        int rc, oid;
        ImagePool * imp = static_cast<ImagePool *>(pool);

        // Allocate an image.
        rc = imp->allocate(uids[0], templates[0], &oid);
        CPPUNIT_ASSERT( oid == 0 );
        CPPUNIT_ASSERT( oid == rc );

        // Try to allocate twice the same image, should fail
        rc = imp->allocate(uids[0], templates[0], &oid);
        CPPUNIT_ASSERT( rc  == -1 );
        CPPUNIT_ASSERT( oid == rc );

        // Try again, with different uid
        rc = imp->allocate(uids[1], templates[0], &oid);
        CPPUNIT_ASSERT( rc  == -1 );
        CPPUNIT_ASSERT( oid == rc );
    }

    void extra_attributes()
    {
        int     oid;
        string  value = "";
        Image * img;

        // The third template doen't have a description, but has some
        // extra attibutes
        oid = allocate(2);
        CPPUNIT_ASSERT( oid == 0 );

        pool->clean();


        img = ((ImagePool*)pool)->get(oid, false);
        check(2, img);

        img->get_template_attribute("DESCRIPTION", value);
        CPPUNIT_ASSERT( value == "" );

        img->get_template_attribute("PROFILE", value);
        CPPUNIT_ASSERT( value == "STUDENT" );
    }


    void dump()
    {
        ImagePool * imp = static_cast<ImagePool*>(pool);

        ostringstream oss;
        int oid, rc;

        allocate(0);
        allocate(1);
        allocate(2);

        rc = imp->dump(oss, "");
        CPPUNIT_ASSERT(rc == 0);

        string result = oss.str();

        result.replace(88,  10, replacement);
        result.replace(310, 10, replacement);
        result.replace(535, 10, replacement);

        CPPUNIT_ASSERT( result == xml_dump );
    }

    void dump_where()
    {
        ImagePool * imp = static_cast<ImagePool*>(pool);

        int oid, rc;
        ostringstream oss;
        ostringstream where;


        allocate(0);
        allocate(1);
        allocate(2);

        where << "oid > 0";

        rc = imp->dump(oss, where.str());
        CPPUNIT_ASSERT(rc == 0);

        string result = oss.str();

        result.replace(91,  10, replacement);
        result.replace(316, 10, replacement);

        CPPUNIT_ASSERT( result == xml_dump_where );
    }

    /* ********************************************************************* */

};

/* ************************************************************************* */
/* ************************************************************************* */

int main(int argc, char ** argv)
{
    return PoolTest::main(argc, argv, ImagePoolTest::suite());
}
