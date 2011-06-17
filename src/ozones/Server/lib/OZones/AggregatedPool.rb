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
    
    class AggregatedPool 
        include  OpenNebulaJSON::JSONUtils 
         
        def initialize(tag)
            @tag                          = tag
        end
    
        def info
            @sup_aggregated_pool          = Hash.new
            @sup_aggregated_pool[@tag]    = Hash.new
        
            OZones::Zones.all.each{|zone|
                client   = OpenNebula::Client.new(
                                      zone.onename + ":" + zone.onepass,
                                      zone.endpoint)

                zone_tag = zone[:id].to_s                        
                pool = factory(client)  
                
                if OpenNebula.is_error?(pool)
                    @sup_aggregated_pool[@tag][zone_tag] = pool.to_hash
                    next
                end
                     
                rc = pool.info

                if !rc  
                    @sup_aggregated_pool[@tag][zone_tag] = pool.to_hash        
                else
                    @sup_aggregated_pool[@tag][zone_tag] = rc.to_hash 
                end

            }
        end    
    
        def to_hash
            info
            return @sup_aggregated_pool
        end
    end
    
end