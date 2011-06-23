# -------------------------------------------------------------------------- #
# Copyright 2002-2011, OpenNebula Project Leads (OpenNebula.org)             #
#                                                                            #
# Licensed under the Apache License, Version 2.0 (the "License"); you may    #
# not use this file except in compliance with the License. You may obtain    #
# a copy of the License at                                                   #
#                                                                            #
# http://www.apache.org/licenses/LICENSE-2.0                                 #
#                                                                            #
# Unless required by applicable law or agreed to in writing, software        #
# distributed under the License is distributed on an "AS IS" BASIS,          #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   #
# See the License for the specific language governing permissions and        #
# limitations under the License.                                             #
#--------------------------------------------------------------------------- #

class OpenNebulaHM < OpenNebulaNetwork
    def initialize(vm, hypervisor = nil)
        super(vm,hypervisor)
        @bridges = get_interfaces
    end

    def activate
        vm_id =  @vm['ID']
        process do |nic|
            bridge  = nic[:bridge]
            dev     = nic[:phydev]

            if dev
                vlan = CONF[:start_vlan] + nic[:network_id].to_i

                create_bridge bridge if !bridge_exists? bridge

                create_dev_vlan(dev, vlan) if !device_exists?(dev, vlan)

                if !attached_bridge_dev?(bridge, dev, vlan)
                    attach_brigde_dev(bridge, dev, vlan)
                end
            end
        end
    end

    def deactivate
        vm_id =  @vm['ID']
        process do |nic|
        end
    end

    def bridge_exists?(bridge)
        @bridges.keys.include? bridge
    end

    def create_bridge(bridge)
        system("#{COMMANDS[:brctl]} addbr #{bridge}")
    end

    def device_exists?(dev, vlan=nil)
        dev = "#{dev}.#{vlan}" if vlan
        system("#{COMMANDS[:ip]} link show #{dev}")
    end

    def create_dev_vlan(dev, vlan)
        system("#{COMMANDS[:vconfig]} add #{dev} #{vlan}")
    end

    def attached_bridge_dev?(bridge, dev, vlan=nil)
        return false if !bridge_exists? bridge
        dev = "#{dev}.#{vlan}" if vlan
        @bridges[bridge].include? dev
    end

    def attach_brigde_dev(bridge, dev, vlan=nil)
        dev = "#{dev}.#{vlan}" if vlan
        system("#{COMMANDS[:brctl]} addif #{bridge} #{dev}")
    end
end
