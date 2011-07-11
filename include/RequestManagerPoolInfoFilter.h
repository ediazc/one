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

#ifndef REQUEST_MANAGER_POOL_INFO_FILTER_H_
#define REQUEST_MANAGER_POOL_INFO_FILTER_H_

#include "Request.h"
#include "Nebula.h"
#include "AuthManager.h"

using namespace std;

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

class RequestManagerPoolInfoFilter: public Request
{
protected:
    RequestManagerPoolInfoFilter(const string& method_name,
                                 const string& help,
                                 const string& signature)
        :Request(method_name,signature,help)
    {
        auth_op = AuthRequest::INFO_POOL;
    };

    ~RequestManagerPoolInfoFilter(){};

    /* -------------------------------------------------------------------- */

    static const int ALL;        /**< Specify all objects in the pool (-2)   */
    static const int MINE;       /**< Specify user's objects in the pool (-3)*/
    static const int MINE_GROUP; /**< Specify users + group objects (-1)     */

    /* -------------------------------------------------------------------- */

    void request_execute(xmlrpc_c::paramList const& _paramList,
                         RequestAttributes& att);
};

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

class VirtualMachinePoolInfo : public RequestManagerPoolInfoFilter
{
public:
    /* -------------------------------------------------------------------- */

    static const int ALL_VM;   /**< VMs in any state  (-2) */
    static const int NOT_DONE; /**< VMs in any state expect DONE (-1)*/

    /* -------------------------------------------------------------------- */

    VirtualMachinePoolInfo():
        RequestManagerPoolInfoFilter("VirtualMachinePoolInfo",
                                     "Returns the virtual machine instances pool",
                                     "A:siiii")
    {    
        Nebula& nd  = Nebula::instance();
        pool        = nd.get_vmpool();
        auth_object = AuthRequest::VM;
    };

    ~VirtualMachinePoolInfo(){};
};

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

class TemplatePoolInfo : public RequestManagerPoolInfoFilter
{
public:
    TemplatePoolInfo():
        RequestManagerPoolInfoFilter("TemplatePoolInfo",
                                     "Returns the virtual machine template pool",
                                     "A:siii")
    {    
        Nebula& nd  = Nebula::instance();
        pool        = nd.get_tpool();
        auth_object = AuthRequest::TEMPLATE;
    };

    ~TemplatePoolInfo(){};
};

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

class VirtualNetworkPoolInfo: public RequestManagerPoolInfoFilter
{
public:
    VirtualNetworkPoolInfo():
        RequestManagerPoolInfoFilter("VirtualNetworkPoolInfo",
                                     "Returns the virtual network pool",
                                     "A:siii")
    {    
        Nebula& nd  = Nebula::instance();
        pool        = nd.get_vnpool();
        auth_object = AuthRequest::NET;
    };

    ~VirtualNetworkPoolInfo(){};
};

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

class ImagePoolInfo: public RequestManagerPoolInfoFilter
{
public:
    ImagePoolInfo():
        RequestManagerPoolInfoFilter("ImagePoolInfo",
                                     "Returns the image pool",
                                     "A:siii")
    {    
        Nebula& nd  = Nebula::instance();
        pool        = nd.get_ipool();
        auth_object = AuthRequest::IMAGE;
    };

    ~ImagePoolInfo(){};
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

#endif
