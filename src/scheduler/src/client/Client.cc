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

#include "Client.h"

#include <fstream>
#include <pwd.h>
#include <stdlib.h>
#include <stdexcept>

#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <sstream>

#include <openssl/evp.h>
#include <iomanip>

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

const int Client::MESSAGE_SIZE = 51200;
//
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void Client::set_one_auth(string secret)
{
    string user = "";
    string pass = "";

    int rc = 0;

    if( secret == "" )
    {
        rc = read_oneauth(secret);
    }

    if ( rc == 0 )
    {
        rc = split_secret(secret, user, pass);

        if( rc == 0 )
        {
	    string plain_tok = "plain:";
            if(pass.find(plain_tok) == 0)
	    {
	        size_t pt_length = plain_tok.length();
                if(pass.length() == pt_length)
		{
		    throw runtime_error("Empty password for auth token in the form "
                                    "<username>:plain:<password>");
		}
		else
		{
		    string plain_pass = pass.substr(pt_length);
                    one_auth = user + ":" + plain_pass;
		}
	    }
	    else
	    {
	        string sha1_pass = sha1_digest(pass);
                one_auth = user + ":" + sha1_pass;
	    }
        }
        else
        {
            throw runtime_error("Wrong format for auth token, must "
                                "be <username>:<passwd>");
        }
    }
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int Client::read_oneauth(string &secret)
{
    ostringstream oss;
    string        one_auth_file;

    const char *  one_auth_env;
    ifstream      file;

    bool rc = -1;

    // Read $ONE_AUTH file and copy its contents into secret.
    one_auth_env = getenv("ONE_AUTH");

    if (!one_auth_env)
    {
        // If $ONE_AUTH doesn't exist, read $HOME/.one/one_auth
        struct passwd * pw_ent;

        pw_ent = getpwuid(getuid());

        if ((pw_ent != NULL) && (pw_ent->pw_dir != NULL))
        {
            one_auth_file = pw_ent->pw_dir;
            one_auth_file += "/.one/one_auth";

            one_auth_env = one_auth_file.c_str();
        }
        else
        {
            oss << "Could not get one_auth file location";
        }
    }

    file.open(one_auth_env);

    if (file.good())
    {
        getline(file, secret);

        if (file.fail())
        {
            oss << "Error reading file: " << one_auth_env;
        }
        else
        {
            rc = 0;
        }
    }
    else
    {
        oss << "Could not open file: " << one_auth_env;
    }

    file.close();

    if (rc != 0)
    {
        NebulaLog::log("XMLRPC",Log::ERROR,oss);
    }

    return rc;
}


int Client::split_secret(const string secret, string& user, string& pass)
{
    size_t pos;
    int    rc = -1;

    pos=secret.find(":");

    if (pos != string::npos)
    {
        user = secret.substr(0,pos);
        pass = secret.substr(pos+1);

        rc = 0;
    }

    return rc;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

string Client::sha1_digest(const string& pass)
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

void Client::set_one_endpoint(string endpoint)
{
    one_endpoint = "http://localhost:2633/RPC2";

    if(endpoint != "")
    {
        one_endpoint = endpoint;
    }
    else
    {
        char *  xmlrpc_env;
        xmlrpc_env = getenv("ONE_XMLRPC");

        if ( xmlrpc_env != 0 )
        {
            one_endpoint = xmlrpc_env;
        }
    }

    // TODO Check url format, and log error (if any)
}



