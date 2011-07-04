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


#include <stdexcept>
#include <stdlib.h>


#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pwd.h>

#include <pthread.h>

#include <cmath>

#include "Scheduler.h"
#include "RankPolicy.h"
#include "NebulaLog.h"

using namespace std;

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

extern "C" void * scheduler_action_loop(void *arg)
{
    Scheduler *  sched;

    if ( arg == 0 )
    {
        return 0;
    }

    sched = static_cast<Scheduler *>(arg);

    NebulaLog::log("SCHED",Log::INFO,"Scheduler loop started.");

    sched->am.loop(sched->timer,0);

    NebulaLog::log("SCHED",Log::INFO,"Scheduler loop stopped.");

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void Scheduler::start()
{
    int      rc;
    ifstream file;

    pthread_attr_t  pattr;

    // -----------------------------------------------------------
    // Log system
    // -----------------------------------------------------------

    try
    {
        ostringstream oss;
        const char *  nl = getenv("ONE_LOCATION");

        if (nl == 0) //OpenNebula installed under root directory
        {
            oss << "/var/log/one/";
        }
        else
        {
            oss << nl << "/var/";
        }

        oss << "sched.log";

        NebulaLog::init_log_system(NebulaLog::FILE,
                                   Log::DEBUG,
                                   oss.str().c_str());

        NebulaLog::log("SCHED", Log::INFO, "Init Scheduler Log system");
    }
    catch(runtime_error &)
    {
        throw;
    }

    // -----------------------------------------------------------
    // XML-RPC Client
    // -----------------------------------------------------------

    try
    {
        client = new Client("",url);
    }
    catch(runtime_error &)
    {
        throw;
    }


    xmlInitParser();

    // -----------------------------------------------------------
    // Pools
    // -----------------------------------------------------------

    hpool  = new HostPoolXML(client);
    vmpool = new VirtualMachinePoolXML(client, machines_limit);
    upool  = new UserPoolXML(client);

    // -----------------------------------------------------------
    // Load scheduler policies
    // -----------------------------------------------------------

    register_policies();

    // -----------------------------------------------------------
    // Close stds, we no longer need them
    // -----------------------------------------------------------

    int fd;

    fd = open("/dev/null", O_RDWR);

    dup2(fd,0);
    dup2(fd,1);
    dup2(fd,2);

    close(fd);

    fcntl(0,F_SETFD,0); // Keep them open across exec funcs
    fcntl(1,F_SETFD,0);
    fcntl(2,F_SETFD,0);

    // -----------------------------------------------------------
    // Block all signals before creating any  thread
    // -----------------------------------------------------------

    sigset_t    mask;
    int         signal;

    sigfillset(&mask);

    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    // -----------------------------------------------------------
    // Create the scheduler loop
    // -----------------------------------------------------------

    NebulaLog::log("SCHED",Log::INFO,"Starting scheduler loop...");

    pthread_attr_init (&pattr);
    pthread_attr_setdetachstate (&pattr, PTHREAD_CREATE_JOINABLE);

    rc = pthread_create(&sched_thread,&pattr,scheduler_action_loop,(void *) this);

    if ( rc != 0 )
    {
        NebulaLog::log("SCHED",Log::ERROR,
            "Could not start scheduler loop, exiting");

        return;
    }

    // -----------------------------------------------------------
    // Wait for a SIGTERM or SIGINT signal
    // -----------------------------------------------------------

    sigemptyset(&mask);

    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    sigwait(&mask, &signal);

    am.trigger(ActionListener::ACTION_FINALIZE,0); //Cancel sched loop

    pthread_join(sched_thread,0);

    xmlCleanupParser();

    NebulaLog::finalize_log_system();
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int Scheduler::set_up_pools()
{
    int                             rc;
    ostringstream                   oss;
    map<int,int>::const_iterator    it;
    map<int, int>                   shares;

    //--------------------------------------------------------------------------
    //Cleans the cache and get the hosts ids
    //--------------------------------------------------------------------------

    rc = hpool->set_up();

    if ( rc != 0 )
    {
        return rc;
    }

    //--------------------------------------------------------------------------
    //Cleans the cache and get the pending VMs
    //--------------------------------------------------------------------------

    rc = vmpool->set_up();

    if ( rc != 0 )
    {
        return rc;
    }

    //--------------------------------------------------------------------------
    //Cleans the cache and get the users
    //--------------------------------------------------------------------------

    rc = upool->set_up();

    if ( rc != 0 )
    {
        return rc;
    }

    //--------------------------------------------------------------------------
    //Cleans the cache and get the ACLs
    //--------------------------------------------------------------------------

    //TODO
    //  1.- one.acl.list
    //  2.- from_xml

    //--------------------------------------------------------------------------
    //Get the matching hosts for each VM
    //--------------------------------------------------------------------------

    match();

    return 0;
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void Scheduler::match()
{
    VirtualMachineXML * vm;
    int                 vm_memory;
    int                 vm_cpu;
    int                 vm_disk;
    int                 uid;
    string              reqs;

    HostXML * host;
    int       host_memory;
    int       host_cpu;
    char *    error;
    bool      matched;

    UserXML * user;
    set<int>  gids;

    int       rc;

    map<int, ObjectXML*>::const_iterator  vm_it;
    map<int, ObjectXML*>::const_iterator  h_it;

    const map<int, ObjectXML*> pending_vms = vmpool->get_objects();
    const map<int, ObjectXML*> hosts = hpool->get_objects();


    for (vm_it=pending_vms.begin(); vm_it != pending_vms.end(); vm_it++)
    {
        vm = static_cast<VirtualMachineXML*>(vm_it->second);

        reqs = vm->get_requirements();
        uid  = vm->get_uid();

        for (h_it=hosts.begin(), matched=false; h_it != hosts.end(); h_it++)
        {
            host = static_cast<HostXML *>(h_it->second);

            // -----------------------------------------------------------------
            // Evaluate VM requirements
            // -----------------------------------------------------------------

            if (reqs != "")
            {
                rc = host->eval_bool(reqs,matched,&error);

                if ( rc != 0 )
                {
                    ostringstream oss;

                    matched = false;

                    oss << "Error evaluating expresion: " << reqs
                    << ", error: " << error;
                    NebulaLog::log("SCHED",Log::ERROR,oss);

                    free(error);
                }
            }
            else
            {
                matched = true;
            }
            
            if ( matched == false )
            {
                continue;
            }
            
            // -----------------------------------------------------------------
            // Check host capacity
            // -----------------------------------------------------------------

            user    = upool->get(uid);
            matched = false;

            if ( user != 0 )
            {
               set<int> groups = user->get_groups(); 
               //TODO Authorization test for this user on this host   
               //  1.- user = uid
               //  2.- gid
               //  3.- groups
               //  4.- DEPLOY on host->get_hid
            }
            else
            {
                //TODO Log debug info (user not authorized)?
                continue;
            }

            if ( matched == false )
            {
                continue;
            }
            // -----------------------------------------------------------------
            // Check host capacity
            // -----------------------------------------------------------------

            vm->get_requirements(vm_cpu,vm_memory,vm_disk);

            host->get_capacity(host_cpu, host_memory, threshold);

            if ((vm_memory <= host_memory) && (vm_cpu <= host_cpu))
            {
                if (host->test_capacity(vm_cpu,vm_memory,vm_disk) == true)
                {
                	vm->add_host(host->get_hid());
                }
            }
        }
    }
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static float sum_operator (float i, float j)
{
    return i+j;
}

/* -------------------------------------------------------------------------- */

int Scheduler::schedule()
{
    vector<SchedulerHostPolicy *>::iterator it;

    VirtualMachineXML * vm;
    ostringstream       oss;

    vector<float>   total;
    vector<float>   policy;

    map<int, ObjectXML*>::const_iterator  vm_it;

    const map<int, ObjectXML*> pending_vms = vmpool->get_objects();

    for (vm_it=pending_vms.begin(); vm_it != pending_vms.end(); vm_it++)
    {
        vm = static_cast<VirtualMachineXML*>(vm_it->second);

        total.clear();

        for ( it=host_policies.begin();it!=host_policies.end();it++)
        {
            policy = (*it)->get(vm);

            if (total.empty() == true)
            {
                total = policy;
            }
            else
            {
                transform(
                    total.begin(),
                    total.end(),
                    policy.begin(),
                    total.begin(),
                    sum_operator);
            }
        }

        vm->set_priorities(total);
    }

    return 0;
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void Scheduler::dispatch()
{
    VirtualMachineXML * vm;
    ostringstream       oss;

    int             hid;
    int             rc;
    unsigned int    dispatched_vms;

    map<int, ObjectXML*>::const_iterator  vm_it;
    const map<int, ObjectXML*>            pending_vms = vmpool->get_objects();

    map<int, int>  host_vms;

    oss << "Select hosts" << endl;
    oss << "\tPRI\tHID" << endl;
    oss << "\t-------------------" << endl;

    for (vm_it=pending_vms.begin(); vm_it != pending_vms.end(); vm_it++)
    {
        vm = static_cast<VirtualMachineXML*>(vm_it->second);

        oss << "Virtual Machine: " << vm->get_oid() << "\n" << *vm << endl;
    }

    NebulaLog::log("SCHED",Log::INFO,oss);

    dispatched_vms = 0;
    for (vm_it=pending_vms.begin();
         vm_it != pending_vms.end() && ( dispatch_limit <= 0 ||
                                         dispatched_vms < dispatch_limit );
         vm_it++)
    {
        vm = static_cast<VirtualMachineXML*>(vm_it->second);

        rc = vm->get_host(hid,hpool,host_vms,host_dispatch_limit);

        if (rc == 0)
        {
            rc = vmpool->dispatch(vm_it->first,hid);

            if (rc == 0)
            {
                dispatched_vms++;
            }
        }
    }
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void Scheduler::do_action(const string &name, void *args)
{
    int rc;

    if (name == ACTION_TIMER)
    {
        rc = set_up_pools();

        if ( rc != 0 )
        {
            return;
        }

        rc = schedule();

        if ( rc != 0 )
        {
            return;
        }

        dispatch();
    }
    else if (name == ACTION_FINALIZE)
    {
        NebulaLog::log("SCHED",Log::INFO,"Stopping the scheduler...");
    }
}
