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

#include "RequestManager.h"
#include "NebulaLog.h"
#include <cerrno>

#include "RequestManagerPoolInfoFilter.h"
#include "RequestManagerPoolInfo.h"
#include "RequestManagerInfo.h"
#include "RequestManagerDelete.h"
#include "RequestManagerPublish.h"
#include "RequestManagerAllocate.h"
#include "RequestManagerUpdateTemplate.h"
#include "RequestManagerUser.h"

#include "RequestManagerVirtualNetwork.h"
#include "RequestManagerVirtualMachine.h"
#include "RequestManagerVMTemplate.h"

#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h> 
#include <cstring>
   
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

extern "C" void * rm_action_loop(void *arg)
{
    RequestManager *  rm;

    if ( arg == 0 )
    {
        return 0;
    }

    NebulaLog::log("ReM",Log::INFO,"Request Manager started.");

    rm = static_cast<RequestManager *>(arg);
    
    rm->am.loop(0,0);

    NebulaLog::log("ReM",Log::INFO,"Request Manager stopped.");
    
    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
    
extern "C" void * rm_xml_server_loop(void *arg)
{
    RequestManager *    rm;
        
    if ( arg == 0 )
    {
        return 0;
    }

    rm = static_cast<RequestManager *>(arg);
 
    // Set cancel state for the thread
    
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,0);

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,0);
      
    //Start the server
            
    rm->AbyssServer = new xmlrpc_c::serverAbyss(xmlrpc_c::serverAbyss::constrOpt()
        .registryP(&rm->RequestManagerRegistry)
        .logFileName(rm->xml_log_file)
        .socketFd(rm->socket_fd));
        
    rm->AbyssServer->run();

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int RequestManager::setup_socket()
{
    int                 rc;
    int                 yes = 1;
    struct sockaddr_in  rm_addr;
    
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if ( socket_fd == -1 )
    {
        ostringstream oss;

        oss << "Can not open server socket: " << strerror(errno);
        NebulaLog::log("ReM",Log::ERROR,oss);
       
        return -1; 
    }
  
    rc = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)); 

    if ( rc == -1 )
    {
        ostringstream oss;

        oss << "Can not set socket options: " << strerror(errno);
        NebulaLog::log("ReM",Log::ERROR,oss);
        
        close(socket_fd);
               
        return -1;        
    }
    
    fcntl(socket_fd,F_SETFD,FD_CLOEXEC); // Close socket in MADs
    
    rm_addr.sin_family      = AF_INET;
    rm_addr.sin_port        = htons(port);
    rm_addr.sin_addr.s_addr = INADDR_ANY;

    rc = bind(socket_fd,(struct sockaddr *) &(rm_addr),sizeof(struct sockaddr));

    if ( rc == -1) 
    {
        ostringstream oss;

        oss << "Can not bind to port " << port << " : " << strerror(errno);
        NebulaLog::log("ReM",Log::ERROR,oss);
       
        close(socket_fd);
            
        return -1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int RequestManager::start()
{
    pthread_attr_t  pattr;
    ostringstream   oss;
    
    NebulaLog::log("ReM",Log::INFO,"Starting Request Manager...");
    
    int rc = setup_socket();
    
    if ( rc != 0 )
    {
        return -1;
    }
    
    register_xml_methods();
    
    pthread_attr_init (&pattr);
    pthread_attr_setdetachstate (&pattr, PTHREAD_CREATE_JOINABLE);
    
    pthread_create(&rm_thread,&pattr,rm_action_loop,(void *)this);
    
    pthread_attr_init (&pattr);
    pthread_attr_setdetachstate (&pattr, PTHREAD_CREATE_JOINABLE);
    
    oss << "Starting XML-RPC server, port " << port << " ...";
    NebulaLog::log("ReM",Log::INFO,oss);
    
    pthread_create(&rm_xml_server_thread,&pattr,rm_xml_server_loop,(void *)this);

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
  
void RequestManager::do_action(
        const string &  action,
        void *          arg)
{
    if (action == ACTION_FINALIZE)
    {
        NebulaLog::log("ReM",Log::INFO,"Stopping Request Manager...");
        
        pthread_cancel(rm_xml_server_thread); 

        pthread_join(rm_xml_server_thread,0);

        NebulaLog::log("ReM",Log::INFO,"XML-RPC server stopped.");

        delete AbyssServer;
        
        if ( socket_fd != -1 )
        {
            close(socket_fd);
        }
    }
    else
    {
        ostringstream oss;
        oss << "Unknown action name: " << action;
        
        NebulaLog::log("ReM", Log::ERROR, oss);
    }    
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
        
void RequestManager::register_xml_methods()
{
    // User Methods
    xmlrpc_c::methodPtr user_change_password(new UserChangePassword());
    xmlrpc_c::methodPtr user_add_group(new UserAddGroup());
    xmlrpc_c::methodPtr user_del_group(new UserDelGroup());

    // VirtualMachine Template Methods
    xmlrpc_c::methodPtr template_instantiate(new VMTemplateInstantiate());

    // VirtualMachine Methods
    xmlrpc_c::methodPtr vm_deploy(new VirtualMachineDeploy());
    xmlrpc_c::methodPtr vm_migrate(new VirtualMachineMigrate());
    xmlrpc_c::methodPtr vm_action(new VirtualMachineAction()); 
    xmlrpc_c::methodPtr vm_savedisk(new VirtualMachineSaveDisk());

    // VirtualNetwork Methods
    xmlrpc_c::methodPtr vn_addleases(new VirtualNetworkAddLeases());
    xmlrpc_c::methodPtr vn_rmleases(new VirtualNetworkRemoveLeases());

    // Update Template Methods
    xmlrpc_c::methodPtr image_update(new ImageUpdateTemplate());
    xmlrpc_c::methodPtr template_update(new TemplateUpdateTemplate());
    xmlrpc_c::methodPtr host_update(new HostUpdateTemplate());

    // Allocate Methods
    xmlrpc_c::methodPtr vm_allocate(new VirtualMachineAllocate());
    xmlrpc_c::methodPtr image_allocate(new ImageAllocate());
    xmlrpc_c::methodPtr vn_allocate(new VirtualNetworkAllocate());
    xmlrpc_c::methodPtr group_allocate(new GroupAllocate());
    xmlrpc_c::methodPtr template_allocate(new TemplateAllocate());
    xmlrpc_c::methodPtr host_allocate(new HostAllocate());
    xmlrpc_c::methodPtr user_allocate(new  UserAllocate());

    // Publish Methods
    xmlrpc_c::methodPtr template_publish(new TemplatePublish());
    xmlrpc_c::methodPtr vn_publish(new VirtualNetworkPublish());
    xmlrpc_c::methodPtr image_publish(new ImagePublish());

    // Delete Methods
    xmlrpc_c::methodPtr host_delete(new HostDelete());
    xmlrpc_c::methodPtr template_delete(new TemplateDelete());
    xmlrpc_c::methodPtr group_delete(new GroupDelete());
    xmlrpc_c::methodPtr vn_delete(new VirtualNetworkDelete());
    xmlrpc_c::methodPtr user_delete(new UserDelete());
    xmlrpc_c::methodPtr image_delete(new ImageDelete());

    // Info Methods
    xmlrpc_c::methodPtr vm_info(new VirtualMachineInfo());
    xmlrpc_c::methodPtr host_info(new HostInfo());
    xmlrpc_c::methodPtr template_info(new TemplateInfo());
    xmlrpc_c::methodPtr group_info(new GroupInfo());
    xmlrpc_c::methodPtr vn_info(new VirtualNetworkInfo());
    xmlrpc_c::methodPtr user_info(new UserInfo());
    xmlrpc_c::methodPtr image_info(new ImageInfo());

    // PoolInfo Methods 

    xmlrpc_c::methodPtr hostpool_info(new HostPoolInfo());
    xmlrpc_c::methodPtr grouppool_info(new GroupPoolInfo());
    xmlrpc_c::methodPtr userpool_info(new UserPoolInfo());

    // PoolInfo Methods with Filtering

    xmlrpc_c::methodPtr vm_pool_info(new VirtualMachinePoolInfo());
    xmlrpc_c::methodPtr template_pool_info(new TemplatePoolInfo());
    xmlrpc_c::methodPtr vnpool_info(new VirtualNetworkPoolInfo());
    xmlrpc_c::methodPtr imagepool_info(new ImagePoolInfo());

/*     
    xmlrpc_c::methodPtr vm_chown(new
        RequestManager::GenericChown(this,AuthRequest::VM));


    xmlrpc_c::methodPtr template_chown(new
        RequestManager::GenericChown(this,AuthRequest::TEMPLATE));


    xmlrpc_c::methodPtr host_enable(new 
        RequestManager::HostEnable(hpool,upool));

    xmlrpc_c::methodPtr vn_chown(new
        RequestManager::GenericChown(this,AuthRequest::NET));



    xmlrpc_c::methodPtr user_chown(new
        RequestManager::GenericChown(this,USER));
        
    xmlrpc_c::methodPtr image_persistent(new    
        RequestManager::ImagePersistent(ipool, upool));
        
    xmlrpc_c::methodPtr image_enable(new    
        RequestManager::ImageEnable(ipool, upool));

    xmlrpc_c::methodPtr image_chown(new
        RequestManager::GenericChown(this,IMAGE));
*/
    /* VM related methods  */    
/*        

    RequestManagerRegistry.addMethod("one.vm.chown", vm_chown);
*/
    RequestManagerRegistry.addMethod("one.vm.deploy", vm_deploy);
    RequestManagerRegistry.addMethod("one.vm.action", vm_action);
    RequestManagerRegistry.addMethod("one.vm.migrate", vm_migrate);
    RequestManagerRegistry.addMethod("one.vm.savedisk", vm_savedisk);
    RequestManagerRegistry.addMethod("one.vm.allocate", vm_allocate);
    RequestManagerRegistry.addMethod("one.vm.info", vm_info);

    RequestManagerRegistry.addMethod("one.vmpool.info", vm_pool_info);

    /* VM Template related methods*/
/*
    RequestManagerRegistry.addMethod("one.template.chown", template_chown);
*/

    RequestManagerRegistry.addMethod("one.template.update", template_update);
    RequestManagerRegistry.addMethod("one.template.instantiate",template_instantiate);
    RequestManagerRegistry.addMethod("one.template.allocate",template_allocate);
    RequestManagerRegistry.addMethod("one.template.publish", template_publish);
    RequestManagerRegistry.addMethod("one.template.delete", template_delete);
    RequestManagerRegistry.addMethod("one.template.info", template_info);

    RequestManagerRegistry.addMethod("one.templatepool.info",template_pool_info);

    /* Host related methods*/
/*     
    RequestManagerRegistry.addMethod("one.host.enable", host_enable);
*/    
    RequestManagerRegistry.addMethod("one.host.update", host_update);
    RequestManagerRegistry.addMethod("one.host.allocate", host_allocate);   
    RequestManagerRegistry.addMethod("one.host.delete", host_delete);
    RequestManagerRegistry.addMethod("one.host.info", host_info);

    RequestManagerRegistry.addMethod("one.hostpool.info", hostpool_info); 

    /* Group related methods */
//    RequestManagerRegistry.addMethod("one.group.chown",     group_chown);
    RequestManagerRegistry.addMethod("one.group.allocate",  group_allocate);
    RequestManagerRegistry.addMethod("one.group.delete",    group_delete);
    RequestManagerRegistry.addMethod("one.group.info",      group_info);

    RequestManagerRegistry.addMethod("one.grouppool.info",  grouppool_info);

    /* Network related methods*/
/*
    RequestManagerRegistry.addMethod("one.vn.chown", vn_chown);
*/
    RequestManagerRegistry.addMethod("one.vn.addleases", vn_addleases);
    RequestManagerRegistry.addMethod("one.vn.rmleases", vn_rmleases);
    RequestManagerRegistry.addMethod("one.vn.allocate", vn_allocate);   
    RequestManagerRegistry.addMethod("one.vn.publish", vn_publish);
    RequestManagerRegistry.addMethod("one.vn.delete", vn_delete);
    RequestManagerRegistry.addMethod("one.vn.info", vn_info); 

    RequestManagerRegistry.addMethod("one.vnpool.info", vnpool_info); 
    
    
    /* User related methods*/
/*        
    RequestManagerRegistry.addMethod("one.user.chown", user_chown);
*/
    RequestManagerRegistry.addMethod("one.user.allocate", user_allocate);
    RequestManagerRegistry.addMethod("one.user.delete", user_delete);
    RequestManagerRegistry.addMethod("one.user.info", user_info);
    RequestManagerRegistry.addMethod("one.user.addgroup", user_add_group);
    RequestManagerRegistry.addMethod("one.user.delgroup", user_del_group);
    RequestManagerRegistry.addMethod("one.user.passwd", user_change_password);

    RequestManagerRegistry.addMethod("one.userpool.info", userpool_info);
    
    /* Image related methods*/
/*    
    RequestManagerRegistry.addMethod("one.image.persistent", image_persistent);
    RequestManagerRegistry.addMethod("one.image.enable", image_enable);    
    RequestManagerRegistry.addMethod("one.image.chown", image_chown);

*/  
    RequestManagerRegistry.addMethod("one.image.update", image_update);     
    RequestManagerRegistry.addMethod("one.image.allocate", image_allocate);
    RequestManagerRegistry.addMethod("one.image.publish", image_publish);
    RequestManagerRegistry.addMethod("one.image.delete", image_delete);
    RequestManagerRegistry.addMethod("one.image.info", image_info);

    RequestManagerRegistry.addMethod("one.imagepool.info", imagepool_info);
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
        
