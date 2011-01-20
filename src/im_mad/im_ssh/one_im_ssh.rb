#!/usr/bin/env ruby

# -------------------------------------------------------------------------- #
# Copyright 2002-2010, OpenNebula Project Leads (OpenNebula.org)             #
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
require 'CommandManager'

#-------------------------------------------------------------------------------
# The SSH Information Manager Driver
#-------------------------------------------------------------------------------
class InformationManager < OpenNebulaDriver

    #---------------------------------------------------------------------------
    # Init the driver
    #---------------------------------------------------------------------------
    def initialize(hypervisor, num)
        super(num, true)

        @config = read_configuration

        @hypervisor = hypervisor
        @remote_dir = @config['SCRIPTS_REMOTE_DIR']

        # register actions
        register_action(:MONITOR, method("action_monitor"))
    end

    #---------------------------------------------------------------------------
    # Execute the run_probes in the remote host
    #---------------------------------------------------------------------------
    def action_monitor(number, host, do_update)
        log_lambda=lambda do |message|
            log(number, message)
        end

        if do_update == "1"
            # Use SCP to sync:
            sync_cmd = "scp -r #{REMOTES_LOCATION}/. #{host}:#{@remote_dir}"

            # Use rsync to sync:
            # sync_cmd = "rsync -Laz #{REMOTES_LOCATION} #{host}:#{@remote_dir}"
            LocalCommand.run(sync_cmd, log_lambda)
        end

        cmd_string = "#{@remote_dir}/im/run_probes #{@hypervisor} #{host}"      

        cmd = RemotesCommand.run(cmd_string, host, @remote_dir, log_lambda, 2)
    
        if cmd.code == 0
            send_message("MONITOR", RESULT[:success], number, cmd.stdout)
        else
            send_message("MONITOR", RESULT[:failure], number,
                "Could not monitor host #{host}.")
        end
    end

end

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
# Information Manager main program
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------

hypervisor = ARGV[0]||''
im = InformationManager.new(hypervisor, 15)
im.start_driver
