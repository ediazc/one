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

    class Zones
        include DataMapper::Resource
        include OpenNebulaJSON::JSONUtils
        extend  OpenNebulaJSON::JSONUtils

        #######################################################################
        # Data Model for the Zone
        #######################################################################
        property :ID,           Serial
        property :NAME,         String, :required => true, :unique => true
        property :ONENAME,      String, :required => true
        property :ONEPASS,      String, :required => true
        property :ENDPOINT,     String, :required => true
        property :SUNSENDPOINT, String

        has n,   :vdcs

        #######################################################################
        # Constants
        #######################################################################
        ZONE_ATTRS = [:ONENAME, :ONEPASS, :ENDPOINT, :NAME]

        #######################################################################
        # JSON Functions
        #######################################################################
        def self.to_hash
            zonePoolHash = Hash.new
            zonePoolHash["ZONE_POOL"] = Hash.new
            zonePoolHash["ZONE_POOL"]["ZONE"] = Array.new unless self.all.empty?

            self.all.each{|zone|
                zonePoolHash["ZONE_POOL"]["ZONE"] <<
                zone.attributes.merge({:NUMBERVDCS => zone.vdcs.all.size})
            }

            return zonePoolHash
        end

        def to_hash
            zone_attributes = Hash.new
            zone_attributes["ZONE"] = attributes
            zone_attributes["ZONE"][:VDCS] = Array.new

            self.vdcs.all.each{|vdc|
                zone_attributes["ZONE"][:VDCS]<< vdc.attributes
            }

            return zone_attributes
        end

        def ONEPASS
            pw = super
            OZones.decrypt(pw)
        end


        #######################################################################
        # Zone Data Management
        #######################################################################
        def self.create(zone_data)

            ZONE_ATTRS.each { |param|
                if !zone_data[param]
                    return OZones::Error.new("Error: Couldn't create zone. " \
                                             "Mandatory attribute '#{param}' is missing.")
                end
            }

            # Digest and check credentials
            name = zone_data[:ONENAME]
            pass = zone_data[:ONEPASS]

            zone_data[:ONEPASS] = OZones.encrypt(pass)

            rc = OpenNebulaZone::check_oneadmin(name,
                                                pass,
                                                zone_data[:ENDPOINT])

            if OpenNebula.is_error?(rc)
                return OZones::Error.new("Error: Couldn't create zone. "\
                                         "Reason: #{rc.message}")
            end


            # Create the zone
            begin
                zone = Zones.new
                zone.raise_on_save_failure = true

                zone.attributes = zone_data
                zone.save
            rescue => e
                $stderr.puts e.backtrace
                return OZones::Error.new(e.message)
            end

            return zone
        end
    end

    ###########################################################################
    #  This class represents a Zone able to interact with its supporting
    #  OpenNebula installation through OCA. Data persistence is provided by a
    #  Zones class
    ##########################################################################
    class OpenNebulaZone
        def initialize(zoneid)
            @zone = Zones.get(zoneid)

            if !@zone
                raise "Error: Zone with id #{zoneid} not found"
            end

            @client = OpenNebula::Client.new("#{@zone.ONENAME}:#{@zone.ONEPASS}",
                                             @zone.ENDPOINT)
        end

        def pool_to_json(pool_kind)
            pool = case pool_kind
                   when "host"  then
                       OpenNebulaJSON::HostPoolJSON.new(@client)
                   when "image" then
                       OpenNebulaJSON::ImagePoolJSON.new(@client)
                   when "user"  then
                       OpenNebulaJSON::UserPoolJSON.new(@client)
                   when "vm"    then
                       OpenNebulaJSON::VirtualMachinePoolJSON.new(@client)
                   when "vn","vnet" then
                       OpenNebulaJSON::VirtualNetworkPoolJSON.new(@client)
                   when "template","vmtemplate" then
                       OpenNebulaJSON::TemplatePoolJSON.new(@client)
                   else
                       error = OZones::Error.new("Error: Pool #{pool_kind} not " \
                                                 "supported for zone view")
                       return [404, error.to_json]
                   end

            pool.info

            return [200, pool.to_json]
        end

        def self.all_pools_to_json(pool_kind)
            pool = case pool_kind
                   when "host"  then
                       OZones::AggregatedHosts.new
                   when "image" then
                       OZones::AggregatedImages.new
                   when "user"  then
                       OZones::AggregatedUsers.new
                   when "vm"    then
                       OZones::AggregatedVirtualMachines.new
                   when "vn","vnet" then
                       OZones::AggregatedVirtualNetworks.new
                   when "template","vmtemplate" then
                       OZones::AggregatedTemplates.new
                   else
                       error = OZones::Error.new("Error: Pool #{pool_kind} not" \
                                                 " supported for aggregated zone view")
                       return [404, error.to_json]
                   end

            return [200, pool.to_json]
        end


        def self.check_oneadmin(name, pass, endpoint)
            # Create a new client to interact with the zone
            client   = OpenNebula::Client.new("#{name}:#{pass}",endpoint)
            hostpool = OpenNebula::HostPool.new(client)

            return hostpool.info
        end
    end
end
