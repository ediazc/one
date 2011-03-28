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

#include "NebulaTest.h"

NebulaTest* NebulaTest::the_tester;

VirtualMachinePool* NebulaTest::create_vmpool(SqlDB* db, string hook_location)
{
    vector<const Attribute *> hooks;
    return new VirtualMachinePool(db, hooks, hook_location);
}

HostPool* NebulaTest::create_hpool(SqlDB* db, string hook_location)
{
    vector<const Attribute *> hooks;
    return new HostPool(db, hooks, hook_location);
}

VirtualNetworkPool* NebulaTest::create_vnpool(SqlDB* db, string mac_prefix, int size)
{
    return new VirtualNetworkPool(db,mac_prefix,size);
}

UserPool* NebulaTest::create_upool(SqlDB* db)
{
    return new UserPool(db);
}

ImagePool* NebulaTest::create_ipool( SqlDB* db,
                                string repository_path,
                                string default_image_type,
                                string default_device_prefix)
{
    return new ImagePool(db,repository_path,default_image_type,
                         default_device_prefix);
}

ClusterPool* NebulaTest::create_cpool(SqlDB* db)
{
    return new ClusterPool(db);
}

// -----------------------------------------------------------
// Managers
// -----------------------------------------------------------

VirtualMachineManager* NebulaTest::create_vmm(VirtualMachinePool* vmpool,
                                              HostPool*           hpool,
                                              time_t              timer_period,
                                              time_t              poll_period)
{
    vector<const Attribute *> vmm_mads;
    return new VirtualMachineManager(vmpool,
                                     hpool,
                                     timer_period,
                                     poll_period,
                                     vmm_mads);
}

LifeCycleManager* NebulaTest::create_lcm(VirtualMachinePool* vmpool, 
                                         HostPool*           hpool)
{
    return new LifeCycleManager(vmpool,hpool);
}

InformationManager* NebulaTest::create_im(HostPool*   hpool,
                                          time_t      timer_period,
                                          string      remotes_location)
{
    vector<const Attribute *>   im_mads;
    time_t                      monitor_period = 0;

    return new InformationManager(hpool,
                                  timer_period,
                                  monitor_period,
                                  remotes_location,
                                  im_mads);
}

TransferManager* NebulaTest::create_tm(VirtualMachinePool* vmpool,
                                       HostPool*           hpool)
{
    vector<const Attribute *> tm_mads;

    return new TransferManager(vmpool, hpool, tm_mads);
}

DispatchManager* NebulaTest::create_dm(VirtualMachinePool* vmpool,
                                       HostPool*           hpool)
{
    return new DispatchManager(vmpool, hpool);
}

RequestManager* NebulaTest::create_rm(
                VirtualMachinePool *    vmpool,
                HostPool *              hpool,
                VirtualNetworkPool *    vnpool,
                UserPool           *    upool,
                ImagePool          *    ipool,
                ClusterPool        *    cpool,
                string                  log_file)
{
    int rm_port = 2633;

    return new RequestManager(vmpool,
                              hpool,
                              vnpool,
                              upool,
                              ipool,
                              cpool,
                              rm_port,
                              log_file);
}

HookManager* NebulaTest::create_hm(VirtualMachinePool * vmpool)
{
    map<string,string>          mad_value;
    VectorAttribute *           mad;

    vector<const Attribute *>   hm_mads;

    mad_value.insert(make_pair("executable","one_hm"));

    mad = new VectorAttribute("HM_MAD",mad_value);
    hm_mads.push_back(mad);

    return new HookManager(hm_mads,vmpool);
}

AuthManager* NebulaTest::create_authm(time_t timer_period)
{
    return 0;
}
