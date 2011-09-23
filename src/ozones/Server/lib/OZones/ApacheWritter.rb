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

module OZones
    
    class ApacheWritter 
        def initialize(file_path) 
            @file_path = file_path
            
            File.open(@file_path, 'w') {|f| 
                f.flock(File::LOCK_EX)
                f.write(htaccess) 
            }
        end
        
        def update
            htaccess = "RewriteEngine On\n"
                                              
            OZones::Zones.all.each{|zone|
                zone.vdcs.all.each{|vdc|
                    htaccess << "RewriteRule ^(.*)/#{vdc.name} " +
                                "#{zone.endpoint} [P]\n"
                    if zone.sunsendpoint != nil
                        htaccess << "RewriteRule ^(.*)/sunstone_#{vdc.name} " +
                                    "#{zone.sunsendpoint} [P]\n" 
                    end
                }
            }

            File.open(@file_path, 'w') {|f| 
                f.flock(File::LOCK_EX)
                f.write(htaccess) 
            }
        end
    end
    
end