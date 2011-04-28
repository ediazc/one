#!/usr/bin/env ruby

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

ONE_LOCATION=ENV["ONE_LOCATION"]

if !ONE_LOCATION
    RUBY_LIB_LOCATION="/usr/lib/one/ruby"
    ETC_LOCATION="/etc/one/"
    REMOTES_LOCATION="/var/lib/one/remotes"
else
    RUBY_LIB_LOCATION=ONE_LOCATION+"/lib/ruby"
    ETC_LOCATION=ONE_LOCATION+"/etc/"
    REMOTES_LOCATION=ONE_LOCATION+"/var/remotes/"
end

$: << RUBY_LIB_LOCATION

require 'OpenNebulaDriver'
require 'getoptlong'

#-------------------------------------------------------------------------------
# The Local Information Manager Driver
#-------------------------------------------------------------------------------
class InformationManagerDriverSH < OpenNebulaDriver

    #---------------------------------------------------------------------------
    # Init the driver
    #---------------------------------------------------------------------------
    def initialize(hypervisor, num)
        super(num, true, 0)

        @config     = read_configuration
        @hypervisor = hypervisor

        @cmd_path   = "#{REMOTES_LOCATION}/im"

        # register actions
        register_action(:MONITOR, method("action_monitor"))
    end

    #---------------------------------------------------------------------------
    # Execute the run_probes in the remote host
    #---------------------------------------------------------------------------
    def action_monitor(number, host, unused)
        cmd_string  = "#{@cmd_path}/run_probes #{@hypervisor} #{host}"

        local_action(cmd_string, number, "MONITOR")
    end

end

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
# IM Driver main program
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------

opts = GetoptLong.new(
    [ '--threads',    '-t', GetoptLong::OPTIONAL_ARGUMENT ]
)

hypervisor = ''
threads    = 15

begin
    opts.each do |opt, arg|
        case opt
            when '--threads'
                threads = arg.to_i
        end
    end
rescue Exception => e
    exit(-1)
end 

if ARGV.length >= 1 
    hypervisor = ARGV.shift
end

im = InformationManagerDriverSH.new(hypervisor,threads)
im.start_driver
