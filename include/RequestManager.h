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

#ifndef REQUEST_MANAGER_H_
#define REQUEST_MANAGER_H_

#include "ActionManager.h"
#include "VirtualMachinePool.h"
#include "HostPool.h"
#include "UserPool.h"
#include "VirtualNetworkPool.h"
#include "ImagePool.h"
#include "ClusterPool.h"
#include "VMTemplatePool.h"
#include "GroupPool.h"

#include "AuthManager.h"

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

using namespace std;

extern "C" void * rm_action_loop(void *arg);

extern "C" void * rm_xml_server_loop(void *arg);

class RequestManager : public ActionListener
{
public:

    RequestManager(
        VirtualMachinePool *    _vmpool,
        HostPool *              _hpool,
        VirtualNetworkPool *    _vnpool,
        UserPool           *    _upool,
        ImagePool          *    _ipool,
        ClusterPool        *    _cpool,
        VMTemplatePool     *    _tpool,
        GroupPool          *    _gpool,
        int                     _port,
        string                  _xml_log_file)
            :vmpool(_vmpool),hpool(_hpool),vnpool(_vnpool),upool(_upool),
            ipool(_ipool),cpool(_cpool),tpool(_tpool),gpool(_gpool),port(_port),
            socket_fd(-1),xml_log_file(_xml_log_file)
    {
        am.addListener(this);
    };

    ~RequestManager()
    {}
    ;

    /**
     *  This functions starts the associated listener thread (XML server), and
     *  creates a new thread for the Request Manager. This thread will wait in
     *  an action loop till it receives ACTION_FINALIZE.
     *    @return 0 on success.
     */
    int start();

    /**
     *  Gets the thread identification.
     *    @return pthread_t for the manager thread (that in the action loop).
     */
    pthread_t get_thread_id() const
    {
        return rm_thread;
    };

    /**
     *
     */
    void finalize()
    {
        am.trigger(ACTION_FINALIZE,0);
    };

private:

    //--------------------------------------------------------------------------
    // Friends, thread functions require C-linkage
    //--------------------------------------------------------------------------

    friend void * rm_xml_server_loop(void *arg);

    friend void * rm_action_loop(void *arg);

    /**
     *  Thread id for the RequestManager
     */
    pthread_t               rm_thread;

    /**
     *  Thread id for the XML Server
     */
    pthread_t               rm_xml_server_thread;

    /**
     *  Pointer to the VM Pool, to access Virtual Machines
     */
    VirtualMachinePool *    vmpool;

    /**
     *  Pointer to the Host Pool, to access hosts
     */
    HostPool           *    hpool;

    /**
     *  Pointer to the VN Pool, to access Virtual Netowrks
     */
    VirtualNetworkPool *    vnpool;

    /**
     *  Pointer to the User Pool, to access users
     */
    UserPool           *    upool;

    /**
     *  Pointer to the Image Pool, to access images
     */
    ImagePool          *    ipool;

    /**
     *  Pointer to the Cluster Pool, to access clusters
     */
    ClusterPool        *    cpool;

    /**
     *  Pointer to the Template Pool, to access templates
     */
    VMTemplatePool     *    tpool;

    /**
     *  Pointer to the Group Pool, to access groups
     */
    GroupPool          *    gpool;

    /**
     *  Port number where the connection will be open
     */
    int port;

    /*
     *  FD for the XML server socket
     */
    int socket_fd;

    /**
     *  Filename for the log of the xmlrpc server that listens
     */
    string xml_log_file;

    /**
     *  Action engine for the Manager
     */
    ActionManager   am;

    /**
     *  To register XML-RPC methods
     */
    xmlrpc_c::registry RequestManagerRegistry;

    /**
     *  The XML-RPC server
     */
    xmlrpc_c::serverAbyss *  AbyssServer;

    /**
     *  The action function executed when an action is triggered.
     *    @param action the name of the action
     *    @param arg arguments for the action function
     */
    void do_action(const string & action, void * arg);

    void register_xml_methods();

    int setup_socket();

    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
    //                          Error Messages
    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------


    /**
     *  Logs authorization errors
     *    @param method name of the RM method where the error arose
     *    @param action authorization action
     *    @param object object that needs to be authorized
     *    @param uid user that is authorized
     *    @param id id of the object, -1 for Pool
     *    @returns string for logging
     */
    static string authorization_error (const string& method,
                                       const string &action,
                                       const string &object,
                                       int   uid,
                                       int   id)
    {
        ostringstream oss;
        oss << "[" << method << "]" << " User [" << uid << "] not authorized"
            << " to perform " << action << " on " << object;


        if ( id != -1 )
        {
            oss << " [" << id << "].";
        }
        else
        {
            oss << " Pool";
        }

        return oss.str();
    }

    static string authorization_error (const string& method,
                const string &action,AuthRequest::Object ob,int uid,int id)
    {
        return authorization_error(method,action,get_object_name(ob),uid,id);
    }

    /**
     *  Logs authenticate errors
     *    @param method name of the RM method where the error arose
     *    @returns string for logging
     */
    static string authenticate_error (const string& method)
    {
        ostringstream oss;

        oss << "[" << method << "]" << " User couldn't be authenticated," <<
               " aborting call.";

        return oss.str();
    }

    /**
     *  Logs get object errors
     *    @param method name of the RM method where the error arose
     *    @param object over which the get failed
     *    @param id of the object over which the get failed
     *    @returns string for logging
     */
    static string get_error (const string& method,
                             const string &object,
                             int id)
    {
        ostringstream oss;

        oss << "[" << method << "]" << " Error getting " <<
               object;

       if ( id != -1 )
       {
           oss << " [" << id << "].";
       }
       else
       {
          oss << " Pool.";
       }

       return oss.str();
    }

    static string get_error (const string& method,AuthRequest::Object ob,int id)
    {
        return get_error(method,get_object_name(ob),id);
    }

    /**
     *  Logs action errors
     *    @param method name of the RM method where the error arose
     *    @param action that triggered the error
     *    @param object over which the action was applied
     *    @param id id of the object, -1 for Pool, -2 for no-id objects
     *              (allocate error, parse error)
     *    @param rc returned error code (NULL to ignore)
     *    @returns string for logging
     */
    static string action_error (const string& method,
                                const string &action,
                                const string &object,
                                int id,
                                int rc)
    {
        ostringstream oss;

        oss << "[" << method << "]" << " Error trying to " << action << " "
            << object;

        switch(id)
        {
            case -2:
                break;
            case -1:
                oss << "Pool.";
                break;
            default:
                oss << " [" << id << "].";
                break;
        }

        if ( rc != 0 )
        {
            oss << " Returned error code [" << rc << "].";
        }

        return oss.str();
    }

    static string action_error (const string& method,const string &action,
                                AuthRequest::Object ob,int id,int rc)
    {
        return action_error(method, action, get_object_name(ob), id, rc);
    }

    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
    //                          Constants and Helpers
    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------

    PoolSQL * get_pool(AuthRequest::Object ob)
    {
        switch (ob)
        {
            case AuthRequest::VM:       return static_cast<PoolSQL*>(vmpool);
            case AuthRequest::HOST:     return static_cast<PoolSQL*>(hpool);
            case AuthRequest::NET:      return static_cast<PoolSQL*>(vnpool);
            case AuthRequest::IMAGE:    return static_cast<PoolSQL*>(ipool);
            case AuthRequest::USER:     return static_cast<PoolSQL*>(upool);
            case AuthRequest::CLUSTER:  return static_cast<PoolSQL*>(cpool);
            case AuthRequest::TEMPLATE: return static_cast<PoolSQL*>(tpool);
            case AuthRequest::GROUP:    return static_cast<PoolSQL*>(gpool);
        }
    };

    static string get_method_prefix(AuthRequest::Object ob)
    {
        switch (ob)
        {
            case AuthRequest::VM:       return "VirtualMachine";
            case AuthRequest::HOST:     return "Host";
            case AuthRequest::NET:      return "VirtualNetwork";
            case AuthRequest::IMAGE:    return "Image";
            case AuthRequest::USER:     return "User";
            case AuthRequest::CLUSTER:  return "Cluster";
            case AuthRequest::TEMPLATE: return "Template";
            case AuthRequest::GROUP:    return "Group";
        }
    };

    static string get_object_name(AuthRequest::Object ob)
    {
        switch (ob)
        {
            case AuthRequest::VM:       return "VM";
            case AuthRequest::HOST:     return "HOST";
            case AuthRequest::NET:      return "NET";
            case AuthRequest::IMAGE:    return "IMAGE";
            case AuthRequest::USER:     return "USER";
            case AuthRequest::CLUSTER:  return "CLUSTER";
            case AuthRequest::TEMPLATE: return "TEMPLATE";
            case AuthRequest::GROUP:    return "GROUP";
        }
    };

    int add_object_group(
        AuthRequest::Object object_type,
        AuthRequest::Object group_type,
        int                 object_id,
        int                 group_id,
        ostringstream&      oss,
        bool                add=true)
    {
        int rc = 0;

        PoolSQL *   object_pool = get_pool(object_type);
        PoolSQL *   group_pool  = get_pool(group_type);
        string      method_name = get_method_prefix(object_type) + "Add";

        PoolObjectSQL *     object = 0;
        PoolObjectSQL *     group  = 0;

        ObjectCollection *  group_collection  = 0;


        // Get group locked
        group = group_pool->get(group_id,true);

        if ( group == 0 )
        {
            goto error_get_group;
        }

        // Get object locked
        object = object_pool->get(object_id,true);

        if ( object == 0 )
        {
            goto error_get_object;
        }

        // Add/Delete object to the group
        group_collection = group->get_collection();

        if( group_collection == 0 )
        {
            goto error_group_add_del;
        }

        if(add)
        {
            rc = group_collection->add_collection_id(object);
        }
        else
        {
            rc = group_collection->del_collection_id(object);
        }

        if( rc != 0 )
        {
            goto error_group_add_del;
        }

        // Update the DB
        group_pool->update(group);

        object_pool->update(object);

        object->unlock();
        group->unlock();

        return 0;

    error_get_object:
        oss.str(get_error(method_name, object_type, object_id));
        goto error_common;

    error_get_group:
        oss.str(get_error(method_name, group_type, group_id));
        goto error_common;

    error_group_add_del:
        oss.str(action_error(method_name, "MANAGE", group_type, group_id, rc));
        goto error_common;

    error_common:
        if( object != 0 )
        {
            object->unlock();
        }

        if( group != 0 )
        {
            group->unlock();
        }

        return -1;
    }

    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
    //                          XML-RPC Methods
    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------

    /* ---------------------------------------------------------------------- */
    /*                     Generic Helpers                                    */
    /* ---------------------------------------------------------------------- */

    /**
     *  This method takes three arguments: oid, uid and gid. It changes the
     *  owner and/or group id of a PoolSQLObject.
     *  If the arguments (owner or gid) are -1, the value is not updated.
     *
     *  PoolSQLObjects created with an uid or gid of -1 won't update their
     *  values.
     */
    class GenericChown: public xmlrpc_c::method
    {
    public:
        GenericChown(RequestManager  *   _rm,
                     AuthRequest::Object _ob):
                        rm(_rm),
                        ob(_ob)
        {
            _signature="A:siii";
            _help="Changes the owner and/or group";
        };

        ~GenericChown(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        RequestManager *    rm;
        AuthRequest::Object ob;
    };

    /**
     *  This method takes two arguments: object_id and group_id.
     *  It adds/removes the object_id to the group and, if the object has a
     *  collection of IDs (e.g. User objects), the group_id is added/removed
     *  to the object's set as well.
     */
    class GenericAddDelGroup: public xmlrpc_c::method
    {
    public:
        GenericAddDelGroup(
                RequestManager  *   _rm,
                AuthRequest::Object _object_type,
                AuthRequest::Object _group_type,
                bool                _add = true):
                        rm(_rm),
                        object_type(_object_type),
                        group_type(_group_type),
                        add(_add)
        {
            _signature="A:sii";
            _help="Adds the element to the group";
        };

        ~GenericAddDelGroup(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        RequestManager *    rm;
        AuthRequest::Object object_type;
        AuthRequest::Object group_type;
        bool                add;
    };

    /* ---------------------------------------------------------------------- */
    /*                     Virtual Machine Interface                          */
    /* ---------------------------------------------------------------------- */
    class VirtualMachineAllocate: public xmlrpc_c::method
    {
    public:
        VirtualMachineAllocate(
            VirtualMachinePool * _vmpool,
            VirtualNetworkPool * _vnpool,
            ImagePool          * _ipool,
            VMTemplatePool     * _tpool,
            UserPool           * _upool):
        vmpool(_vmpool),
        vnpool(_vnpool),
        ipool(_ipool),
        tpool(_tpool),
        upool(_upool)
        {
            _signature="A:ss";
            _help="Allocates a virtual machine in the pool";
        };

        ~VirtualMachineAllocate(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retval);
    private:
        VirtualMachinePool * vmpool;
        VirtualNetworkPool * vnpool;
        ImagePool          * ipool;
        VMTemplatePool     * tpool;
        UserPool           * upool;
    };

    /* ---------------------------------------------------------------------- */

    class VirtualMachineDeploy: public xmlrpc_c::method
    {
    public:
        VirtualMachineDeploy(
            VirtualMachinePool * _vmpool,
            HostPool           * _hpool,
            UserPool           * _upool):
                vmpool(_vmpool),
                hpool(_hpool),
                upool(_upool)
        {
            _signature="A:sii";
            _help="Deploys a virtual machine";
        };

        ~VirtualMachineDeploy(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retval);

    private:
        VirtualMachinePool * vmpool;
        HostPool           * hpool;
        UserPool           * upool;
    };

    /* ---------------------------------------------------------------------- */

    class VirtualMachineAction: public xmlrpc_c::method
    {
    public:
        VirtualMachineAction(
            VirtualMachinePool * _vmpool,
            UserPool * _upool):
                vmpool(_vmpool),
                upool(_upool)
        {
            _signature="A:ssi";
            _help="Performs an action on a virtual machine";
        };

        ~VirtualMachineAction(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retval);

    private:
        VirtualMachinePool * vmpool;
        UserPool *           upool;
    };

    /* ---------------------------------------------------------------------- */

    class VirtualMachineMigrate: public xmlrpc_c::method
    {
    public:
        VirtualMachineMigrate(
            VirtualMachinePool * _vmpool,
            HostPool *           _hpool,
            UserPool *           _upool):
                vmpool(_vmpool),
                hpool(_hpool),
                upool(_upool)
        {
            _signature="A:siib";
            _help="Migrates a virtual machine";
        };

        ~VirtualMachineMigrate(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retval);

    private:
        VirtualMachinePool * vmpool;
        HostPool *           hpool;
        UserPool *           upool;
    };

    /* ---------------------------------------------------------------------- */

    class VirtualMachineInfo: public xmlrpc_c::method
    {
    public:
        VirtualMachineInfo(
            VirtualMachinePool * _vmpool,
            UserPool           * _upool):
                vmpool(_vmpool),
                upool(_upool)
        {
            _signature="A:si";
            _help="Returns virtual machine information";
        };

        ~VirtualMachineInfo(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retval);

    private:
        VirtualMachinePool * vmpool;
        UserPool           * upool;
    };

    /* ---------------------------------------------------------------------- */

    class VirtualMachineSaveDisk: public xmlrpc_c::method
    {
    public:
        VirtualMachineSaveDisk(
            VirtualMachinePool * _vmpool,
            UserPool           * _upool,
            ImagePool          * _ipool):
                vmpool(_vmpool),
                upool(_upool),
                ipool(_ipool)
        {
            _signature="A:siis";
            _help = "Sets the disk to be saved in a new Image with the given "
                    "name.";
        };

        ~VirtualMachineSaveDisk(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retval);

    private:
        VirtualMachinePool * vmpool;
        UserPool           * upool;
        ImagePool          * ipool;
    };

    /* ---------------------------------------------------------------------- */

    class VirtualMachinePoolInfo: public xmlrpc_c::method
    {
    public:
        VirtualMachinePoolInfo(
            VirtualMachinePool * _vmpool,
            UserPool           * _upool):
                vmpool(_vmpool),
                upool(_upool)
        {
            _signature="A:sii";
            _help="Returns the virtual machine pool";
        };

        ~VirtualMachinePoolInfo(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retval);

    private:
        VirtualMachinePool * vmpool;
        UserPool           *  upool;
    };

    /* ---------------------------------------------------------------------- */
    /*                            Template Interface                          */
    /* ---------------------------------------------------------------------- */

    class TemplateAllocate: public xmlrpc_c::method
    {
    public:
        TemplateAllocate(
            VMTemplatePool * _tpool,
            UserPool * _upool):
                tpool(_tpool),
                upool(_upool)
        {
            _signature="A:ss";
            _help="Allocates a template in the pool";
        };

        ~TemplateAllocate(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        VMTemplatePool * tpool;
        UserPool *       upool;
    };

    /* ---------------------------------------------------------------------- */

    class TemplateDelete: public xmlrpc_c::method
    {
    public:
        TemplateDelete(VMTemplatePool * _tpool,
                       UserPool *       _upool):
                            tpool(_tpool),
                            upool(_upool)
        {
            _signature="A:si";
            _help="Deletes a Template";
        };

        ~TemplateDelete(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        VMTemplatePool * tpool;
        UserPool  *      upool;
    };

    /* ---------------------------------------------------------------------- */

    class TemplateInfo: public xmlrpc_c::method
    {
    public:
        TemplateInfo(VMTemplatePool * _tpool,
                     UserPool *       _upool):
                      tpool(_tpool),
                      upool(_upool)
        {
            _signature="A:si";
            _help="Returns information for a Template";
        };

        ~TemplateInfo(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        VMTemplatePool * tpool;
        UserPool  *      upool;
    };

    /* ---------------------------------------------------------------------- */

    class TemplateUpdate: public xmlrpc_c::method
    {
    public:
        TemplateUpdate(VMTemplatePool * _tpool,
                       UserPool *       _upool):
                        tpool(_tpool),
                        upool(_upool)
        {
            _signature="A:siss";
            _help="Modifies Template attribute";
        };

        ~TemplateUpdate(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        VMTemplatePool * tpool;
        UserPool  *      upool;
    };

    /* ---------------------------------------------------------------------- */

    class TemplateRemoveAttribute: public xmlrpc_c::method
    {
    public:
        TemplateRemoveAttribute(VMTemplatePool * _tpool,
                             UserPool *          _upool):
                        tpool(_tpool),
                        upool(_upool)
        {
            _signature="A:sis";
            _help="Removes Template attribute";
        };

        ~TemplateRemoveAttribute(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        VMTemplatePool * tpool;
        UserPool  *      upool;
    };

    /* ---------------------------------------------------------------------- */

    class TemplatePublish: public xmlrpc_c::method
    {
    public:
        TemplatePublish(VMTemplatePool * _tpool,
                     UserPool *          _upool):
                        tpool(_tpool),
                        upool(_upool)
        {
            _signature="A:sib";
            _help="Publish/Unpublish the Template";
        };

        ~TemplatePublish(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        VMTemplatePool * tpool;
        UserPool  *      upool;
    };

    /* ---------------------------------------------------------------------- */

    class TemplatePoolInfo: public xmlrpc_c::method
    {
    public:
        TemplatePoolInfo(
            VMTemplatePool * _tpool,
            UserPool * _upool):
                tpool(_tpool),
                upool(_upool)
        {
            _signature="A:sii";
            _help="Returns the template pool";
        };

        ~TemplatePoolInfo(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        VMTemplatePool * tpool;
        UserPool *       upool;
    };

    /* ---------------------------------------------------------------------- */
    /*                            Host Interface                              */
    /* ---------------------------------------------------------------------- */

    class HostAllocate: public xmlrpc_c::method
    {
    public:
        HostAllocate(
            HostPool * _hpool,
            UserPool * _upool):
                hpool(_hpool),
                upool(_upool)
        {
            _signature="A:sssss";
            _help="Allocates a host in the pool";
        };

        ~HostAllocate(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        HostPool * hpool;
        UserPool * upool;
    };

    /* ---------------------------------------------------------------------- */

    class HostInfo: public xmlrpc_c::method
    {
    public:
        HostInfo(
            HostPool * _hpool,
            UserPool * _upool):
                hpool(_hpool),
                upool(_upool)
        {
            _signature="A:si";
            _help="Returns host information";
        };

        ~HostInfo(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        HostPool * hpool;
        UserPool * upool;
    };

    /* ---------------------------------------------------------------------- */

    class HostPoolInfo: public xmlrpc_c::method
    {
    public:
        HostPoolInfo(HostPool * _hpool,
                     UserPool * _upool):
            hpool(_hpool),
            upool(_upool)
        {
            _signature="A:s";
            _help="Returns the host pool information";
        };

        ~HostPoolInfo(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        HostPool * hpool;
        UserPool * upool;
    };

    /* ---------------------------------------------------------------------- */

    class HostDelete: public xmlrpc_c::method
    {
    public:
        HostDelete(
            HostPool * _hpool,
            UserPool * _upool):
                hpool(_hpool),
                upool(_upool)
        {
            _signature="A:si";
            _help="Deletes a host from the pool";
        };

        ~HostDelete(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        HostPool * hpool;
        UserPool * upool;
    };

    /* ---------------------------------------------------------------------- */

    class HostEnable: public xmlrpc_c::method
    {
    public:
        HostEnable(
            HostPool * _hpool,
            UserPool * _upool):
                hpool(_hpool),
                upool(_upool)
        {
            _signature="A:sib";
            _help="Enables or disables a host";
        };

        ~HostEnable(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        HostPool * hpool;
        UserPool * upool;
    };

    /* ---------------------------------------------------------------------- */
    /*                      Cluster Interface                                 */
    /* ---------------------------------------------------------------------- */

    class ClusterAllocate: public xmlrpc_c::method
    {
    public:
        ClusterAllocate(
            UserPool *      _upool,
            ClusterPool *   _cpool):
                upool(_upool),
                cpool(_cpool)
        {
            _signature="A:ss";
            _help="Allocates a cluster in the pool";
        };

        ~ClusterAllocate(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        UserPool *      upool;
        ClusterPool *   cpool;
    };

    /* ---------------------------------------------------------------------- */

    class ClusterInfo: public xmlrpc_c::method
    {
    public:
        ClusterInfo(
            UserPool *      _upool,
            ClusterPool *   _cpool):
                upool(_upool),
                cpool(_cpool)
        {
            _signature="A:si";
            _help="Returns cluster information";
        };

        ~ClusterInfo(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        UserPool *      upool;
        ClusterPool *   cpool;
    };

    /* ---------------------------------------------------------------------- */

    class ClusterDelete: public xmlrpc_c::method
    {
    public:
        ClusterDelete(
            UserPool *      _upool,
            ClusterPool *   _cpool):
                upool(_upool),
                cpool(_cpool)
        {
            _signature="A:si";
            _help="Deletes a cluster from the pool";
        };

        ~ClusterDelete(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        UserPool *      upool;
        ClusterPool *   cpool;
    };

    /* ---------------------------------------------------------------------- */

    class ClusterAdd: public xmlrpc_c::method
    {
    public:
        ClusterAdd(
            HostPool *      _hpool,
            UserPool *      _upool,
            ClusterPool *   _cpool):
                hpool(_hpool),
                upool(_upool),
                cpool(_cpool)
        {
            _signature="A:sii";
            _help="Adds a host to a cluster";
        };

        ~ClusterAdd(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        HostPool *      hpool;
        UserPool *      upool;
        ClusterPool *   cpool;
    };

    /* ---------------------------------------------------------------------- */

    class ClusterRemove: public xmlrpc_c::method
    {
    public:
        ClusterRemove(
            HostPool *      _hpool,
            UserPool *      _upool,
            ClusterPool *   _cpool):
                hpool(_hpool),
                upool(_upool),
                cpool(_cpool)
        {
            _signature="A:si";
            _help="Removes a host from its cluster";
        };

        ~ClusterRemove(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        HostPool *      hpool;
        UserPool *      upool;
        ClusterPool *   cpool;
    };

    /* ---------------------------------------------------------------------- */

    class ClusterPoolInfo: public xmlrpc_c::method
    {
    public:
        ClusterPoolInfo(
            UserPool *      _upool,
            ClusterPool *   _cpool):
                upool(_upool),
                cpool(_cpool)
        {
            _signature="A:s";
            _help="Returns the cluster pool information";
        };

        ~ClusterPoolInfo(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        UserPool *      upool;
        ClusterPool *   cpool;
    };


    /* ---------------------------------------------------------------------- */
    /*                      Group Interface                                   */
    /* ---------------------------------------------------------------------- */

    class GroupAllocate: public xmlrpc_c::method
    {
    public:
        GroupAllocate(
            UserPool *      _upool,
            GroupPool *     _gpool):
                upool(_upool),
                gpool(_gpool)
        {
            _signature="A:ss";
            _help="Allocates a group in the pool";
        };

        ~GroupAllocate(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        UserPool *      upool;
        GroupPool *     gpool;
    };

    /* ---------------------------------------------------------------------- */

    class GroupInfo: public xmlrpc_c::method
    {
    public:
        GroupInfo(
            UserPool *      _upool,
            GroupPool *     _gpool):
                upool(_upool),
                gpool(_gpool)
        {
            _signature="A:si";
            _help="Returns group information";
        };

        ~GroupInfo(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        UserPool *      upool;
        GroupPool *     gpool;
    };

    /* ---------------------------------------------------------------------- */

    class GroupDelete: public xmlrpc_c::method
    {
    public:
        GroupDelete(
            UserPool *      _upool,
            GroupPool *     _gpool):
                upool(_upool),
                gpool(_gpool)
        {
            _signature="A:si";
            _help="Deletes a group from the pool";
        };

        ~GroupDelete(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        UserPool *      upool;
        GroupPool *     gpool;
    };

    /* ---------------------------------------------------------------------- */

    class GroupPoolInfo: public xmlrpc_c::method
    {
    public:
        GroupPoolInfo(
            UserPool *      _upool,
            GroupPool *     _gpool):
                upool(_upool),
                gpool(_gpool)
        {
            _signature="A:s";
            _help="Returns the group pool information";
        };

        ~GroupPoolInfo(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        UserPool *      upool;
        GroupPool *     gpool;
    };


    /* ---------------------------------------------------------------------- */
    /*                      Virtual Network Interface                         */
    /* ---------------------------------------------------------------------- */


    class VirtualNetworkAllocate: public xmlrpc_c::method
    {
    public:
        VirtualNetworkAllocate(
            VirtualNetworkPool * _vnpool,
            UserPool *           _upool):
                vnpool(_vnpool),
                upool(_upool)
        {
            _signature="A:ss";
            _help="Creates a virtual network";
        };

        ~VirtualNetworkAllocate(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        VirtualNetworkPool * vnpool;
        UserPool           * upool;
    };

    /* ---------------------------------------------------------------------- */

    class VirtualNetworkInfo: public xmlrpc_c::method
    {
    public:
        VirtualNetworkInfo(
            VirtualNetworkPool * _vnpool,
            UserPool           * _upool):
                 vnpool(_vnpool),
                 upool(_upool)
        {
            _signature="A:si";
            _help="Returns virtual network information";
        };

        ~VirtualNetworkInfo(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        VirtualNetworkPool * vnpool;
        UserPool           * upool;
    };

    /* ---------------------------------------------------------------------- */

    class VirtualNetworkPoolInfo: public xmlrpc_c::method
    {
    public:
        VirtualNetworkPoolInfo(VirtualNetworkPool * _vnpool,
                               UserPool           * _upool):
            vnpool(_vnpool),
            upool(_upool)
        {
            _signature="A:si";
            _help="Returns the virtual network pool information";
        };

        ~VirtualNetworkPoolInfo(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        VirtualNetworkPool * vnpool;
        UserPool           * upool;
    };

    /* ---------------------------------------------------------------------- */

    class VirtualNetworkPublish: public xmlrpc_c::method
    {
    public:
        VirtualNetworkPublish(
            VirtualNetworkPool * _vnpool,
            UserPool           * _upool):
                vnpool(_vnpool),
                upool(_upool)
        {
            _signature="A:sib";
            _help="Enables/Disables a virtual network";
        };

        ~VirtualNetworkPublish(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        VirtualNetworkPool * vnpool;
        UserPool           * upool;
    };

    /* ---------------------------------------------------------------------- */

    class VirtualNetworkDelete: public xmlrpc_c::method
    {
    public:
        VirtualNetworkDelete(
            VirtualNetworkPool * _vnpool,
            UserPool           * _upool):
                vnpool(_vnpool),
                upool(_upool)
        {
            _signature="A:si";
            _help="Deletes a virtual network";
        };

        ~VirtualNetworkDelete(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        VirtualNetworkPool * vnpool;
        UserPool           * upool;
    };

    /* ---------------------------------------------------------------------- */

    class VirtualNetworkAddLeases: public xmlrpc_c::method
    {
    public:
        VirtualNetworkAddLeases(
            VirtualNetworkPool * _vnpool,
            UserPool           * _upool):
                vnpool(_vnpool),
                upool(_upool)
        {
            _signature="A:sis";
            _help="Adds leases to a virtual network";
        };

        ~VirtualNetworkAddLeases(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        VirtualNetworkPool * vnpool;
        UserPool           * upool;
    };

    /* ---------------------------------------------------------------------- */

    class VirtualNetworkRemoveLeases: public xmlrpc_c::method
    {
    public:
        VirtualNetworkRemoveLeases(
            VirtualNetworkPool * _vnpool,
            UserPool           * _upool):
                vnpool(_vnpool),
                upool(_upool)
        {
            _signature="A:sis";
            _help="Removes leases from a virtual network";
        };

        ~VirtualNetworkRemoveLeases(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        VirtualNetworkPool * vnpool;
        UserPool           * upool;
    };
    /* ---------------------------------------------------------------------- */
    /*                      User Management Interface                         */
    /* ---------------------------------------------------------------------- */


    class UserAllocate: public xmlrpc_c::method
    {
    public:
        UserAllocate(UserPool *         _upool,
                     RequestManager *   _rm)
                 :upool(_upool),
                 rm(_rm)
        {
            _signature="A:sss";
            _help="Creates a new user";
        };

        ~UserAllocate(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        UserPool *          upool;
        RequestManager *    rm;
    };

    /* ---------------------------------------------------------------------- */

    class UserDelete: public xmlrpc_c::method
    {
    public:
        UserDelete(UserPool * _upool):upool(_upool)
        {
            _signature="A:si";
            _help="Deletes a user account";
        };

        ~UserDelete(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        UserPool * upool;
    };

    /* ---------------------------------------------------------------------- */

    class UserChangePassword: public xmlrpc_c::method
    {
    public:
        UserChangePassword(UserPool * _upool):upool(_upool)
        {
            _signature="A:sis";
            _help="Changes the password for the given user.";
        };

        ~UserChangePassword(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        UserPool * upool;
    };

    /* ---------------------------------------------------------------------- */

    class UserInfo: public xmlrpc_c::method                                   
    {
    public:                                                                   
        UserInfo(UserPool * _upool):upool(_upool)
        {
            _signature="A:si";
            _help="Returns the information for a user";
        };

        ~UserInfo(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        UserPool * upool;
    };

    /* ---------------------------------------------------------------------- */

    class UserPoolInfo: public xmlrpc_c::method
    {
    public:
        UserPoolInfo(UserPool * _upool):upool(_upool)
        {
            _signature="A:s";
            _help="Returns content of the user pool";
        };

        ~UserPoolInfo(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        UserPool * upool;
    };

    /* ---------------------------------------------------------------------- */
    /*                      Image Pool Interface                              */
    /* ---------------------------------------------------------------------- */

    class ImageAllocate: public xmlrpc_c::method
    {
    public:
        ImageAllocate(ImagePool * _ipool,
                      UserPool * _upool):
                            ipool(_ipool),
                            upool(_upool)
        {
            _signature="A:ss";
            _help="Creates a new image";
        };

        ~ImageAllocate(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        ImagePool * ipool;
        UserPool  * upool;
    };

    /* ---------------------------------------------------------------------- */

    class ImageDelete: public xmlrpc_c::method
    {
    public:
        ImageDelete(ImagePool * _ipool,
                      UserPool * _upool):
                            ipool(_ipool),
                            upool(_upool)
        {
            _signature="A:si";
            _help="Deletes an image";
        };

        ~ImageDelete(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        ImagePool * ipool;
        UserPool  * upool;
    };

    /* ---------------------------------------------------------------------- */

    class ImageInfo: public xmlrpc_c::method
    {
    public:
        ImageInfo(ImagePool * _ipool,
                  UserPool  * _upool):
                      ipool(_ipool),
                      upool(_upool)
        {
            _signature="A:si";
            _help="Returns information for an image";
        };

        ~ImageInfo(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        ImagePool * ipool;
        UserPool  * upool;
    };

    /* ---------------------------------------------------------------------- */

    class ImageUpdate: public xmlrpc_c::method
    {
    public:
        ImageUpdate(ImagePool * _ipool,
                    UserPool  * _upool):
                        ipool(_ipool),
                        upool(_upool)
        {
            _signature="A:siss";
            _help="Modifies image attribute";
        };

        ~ImageUpdate(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        ImagePool * ipool;
        UserPool  * upool;
    };

    /* ---------------------------------------------------------------------- */

    class ImageRemoveAttribute: public xmlrpc_c::method
    {
    public:
        ImageRemoveAttribute(ImagePool * _ipool,
                             UserPool  * _upool):
                        ipool(_ipool),
                        upool(_upool)
        {
            _signature="A:sis";
            _help="Removes image attribute";
        };

        ~ImageRemoveAttribute(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        ImagePool * ipool;
        UserPool  * upool;
    };

    /* ---------------------------------------------------------------------- */

    class ImagePublish: public xmlrpc_c::method
    {
    public:
        ImagePublish(ImagePool * _ipool,
                     UserPool  * _upool):
                        ipool(_ipool),
                        upool(_upool)
        {
            _signature="A:sib";
            _help="Publish/Unpublish the Image";
        };

        ~ImagePublish(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        ImagePool * ipool;
        UserPool  * upool;
    };

    /* ---------------------------------------------------------------------- */

    class ImagePersistent: public xmlrpc_c::method
    {
    public:
        ImagePersistent(ImagePool * _ipool,
                     UserPool  * _upool):
                        ipool(_ipool),
                        upool(_upool)
        {
            _signature="A:sib";
            _help="Make an Image (non)persistent";
        };

        ~ImagePersistent(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        ImagePool * ipool;
        UserPool  * upool;
    };

    /* ---------------------------------------------------------------------- */

    class ImageEnable: public xmlrpc_c::method
    {
    public:
        ImageEnable(ImagePool * _ipool,
                     UserPool  * _upool):
                        ipool(_ipool),
                        upool(_upool)
        {
            _signature="A:sib";
            _help="Enables/Disables the Image";
        };

        ~ImageEnable(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        ImagePool * ipool;
        UserPool  * upool;
    };

    /* ---------------------------------------------------------------------- */

    class ImagePoolInfo: public xmlrpc_c::method
    {
    public:
        ImagePoolInfo(ImagePool * _ipool,
                      UserPool  * _upool):
                            ipool(_ipool),
                            upool(_upool)
        {
            _signature="A:si";
            _help="Returns content of image pool attending to the filter flag";
        };

        ~ImagePoolInfo(){};

        void execute(
            xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP);

    private:
        ImagePool * ipool;
        UserPool  * upool;
    };

};


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

#endif

