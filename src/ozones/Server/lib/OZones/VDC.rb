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
    
    class Vdc 
        include DataMapper::Resource
        include OpenNebulaJSON::JSONUtils
        extend OpenNebulaJSON::JSONUtils

        property :id,           Serial
        property :name,         String, :required => true, :unique => true
        property :group_id,     Integer
        property :vdcadminname, String, :required => true
        property :vdcadmin_id,  Integer
        property :acls,         String
        property :hosts,        String

        belongs_to :zones
        
        def self.to_hash
            zonePoolHash = Hash.new
            zonePoolHash["VDC_POOL"] = Hash.new
            zonePoolHash["VDC_POOL"]["VDC"] = Array.new unless self.all.empty?
            self.all.each{|vdc|
                  zonePoolHash["VDC_POOL"]["VDC"] << vdc.attributes              
            }
            return zonePoolHash
        end
        
        def to_hash
            vdc_attributes = Hash.new
            vdc_attributes["VDC"] = attributes
            return vdc_attributes
        end
    end

    ###########################################################################
    #  This class represents a VDC able to interact with its supporting
    #  OpenNebula installation through OCA. Data persistence is provided by a
    #  Vdc class  
    ###########################################################################
    class OpenNebulaVdc
        #######################################################################
        # Constants
        #######################################################################
        VDC_ATTRS = [:vdcadminname, :vdcadminpass, :name, :hosts]

        attr_reader :vdc
        attr_reader :zone

        #Creates an OpenNebula VDC, using its ID, vdcid and the associated zone
        def initialize(vdcid, zone = nil)
            
            if vdcid != -1 
                @vdc = Vdc.get(vdcid)
                
                if !@vdc
                    raise "VDC with id #{vdcid} not found."
                end

                @zone = OZones::Zones.get(@vdc.zones_id)
            else
                @zone = zone
            end

            @client = OpenNebula::Client.new(
                            "#{@zone.onename}:#{@zone.onepass}",
                            @zone.endpoint,
                            false)
        end

        def to_json
            @vdc.to_json
        end

        #######################################################################
        #
        #######################################################################
        def create(vdc_data)
            #Check and prepare VDC data
            VDC_ATTRS.each { |param|
                if !vdc_data[param]
                    return OZones::Error.new("Error: Couldn't create vdc." \
                                "Mandatory attribute '#{param}' is missing.")
                end
            }

            #Create a vdc record
            @vdc = Vdc.new
  
            vdcpass = Digest::SHA1.hexdigest(vdc_data.delete(:vdcadminpass))
            @vdc.attributes = vdc_data

            # Create a group in the zone with the VDC name
            group = OpenNebula::Group.new(OpenNebula::Group.build_xml, @client)
            rc    = group.allocate(@vdc.name)
        
            return rc if OpenNebula.is_error?(rc)

            @vdc.group_id = group.id

            # Create the VDC admin user in the Zone
            user = OpenNebula::User.new(OpenNebula::User.build_xml, @client)
            rc   = user.allocate(@vdc.vdcadminname, vdcpass)

            return rollback(group, nil, nil, rc) if OpenNebula.is_error?(rc)

            @vdc.vdcadmin_id = user.id

            # Change primary group of the admin user to the VDC group
            rc = user.chgrp(group.id)
            return rollback(group, user, nil, rc) if OpenNebula.is_error?(rc)

            # Add ACLs
            aclp  = OpenNebula::AclPool.new(@client)
            rules = get_acls

            rc, acls_str = create_acls(rules)
            return rollback(group, user,acls_str,rc) if OpenNebula.is_error?(rc)

            @vdc.acls = acls_str

            return true
        end

        def destroy
            # Delete the resources from the VDC
            delete_images
            delete_templates
            delete_vms
            delete_vns
            delete_acls

            # Delete users from a group
            up = OpenNebula::UserPool.new(@client)
            up.info

            up.each{|user|
                if user['GID'].to_i == @vdc.group_id
                    user.delete
                end
            }

            # Delete the group
            OpenNebula::Group.new_with_id(@vdc.group_id, @client).delete

            return @vdc.destroy
        end

        #Cleans bootstrap operations in a zone
        def clean_bootstrap
            delete_acls

            OpenNebula::User.new_with_id(@vdc.vdcadmin_id, @client).delete
            OpenNebula::Group.new_with_id(@vdc.group_id, @client).delete
        end

        def update(host_list)
            # Delete existing host ACLs
            delete_host_acls

            @vdc.acls =~ /((\d+,){#{HOST_ACL_FIRST_ID}}).*/
            newacls   = $1.chop

            # Create new ACLs. TODO Rollback ACL creation
            if !host_list.empty?
                host_acls    = get_host_acls(host_list)
                rc, acls_str = create_acls(host_acls)

                return rc if OpenNebula.is_error?(rc) 

                #Create the new acl string.
                newacls << "," << acls_str
            end


            #Update the VDC Record
            begin
                @vdc.raise_on_save_failure = true
                @vdc.hosts = host_list
                @vdc.acls  = newacls

                @vdc.save
            rescue => e
                return OpenNebula::Error.new(e.message)
            end

            return @vdc.to_json 
        end

        private
        #######################################################################
        # Functions to generate ACL Strings
        #######################################################################
        # The ID of the first host ACL
        HOST_ACL_FIRST_ID = 3

        # This method returns an Array of ACL strings to create them 
        # in the target zone
        def get_acls
            rule_str = Array.new

            # Grant permissions to the group
            rule_str << "@#{@vdc.group_id} VM+NET+IMAGE+TEMPLATE/* " \
                        "CREATE+INFO_POOL_MINE"

            # Grant permissions to the vdc admin
            rule_str << "##{@vdc.vdcadmin_id} USER/* CREATE"
            rule_str << "##{@vdc.vdcadmin_id} USER/@#{@vdc.group_id} " \
                        "MANAGE+DELETE+INFO"

            ###############################################################
            #When more rules are added the class constant HOST_ACL_FIRST_ID
            #must be modified
            ###############################################################

            rule_str.concat(get_host_acls)
        end

        def get_host_acls(host_list = nil)
            rule_str = Array.new

            if host_list == nil
                host_list = @vdc.hosts
            end 

            # Grant permissions to use the vdc hosts
            host_list.split(',').each{|hostid|
                rule_str << "@#{@vdc.group_id} HOST/##{hostid} USE"
            }    

            return rule_str
        end

        #######################################################################
        # Functions to delete resources associated to the VDC
        #######################################################################
        # Deletes ACLs for the hosts
        def delete_host_acls
            @vdc.acls.split(',')[HOST_ACL_FIRST_ID..-1].each{|acl|
                OpenNebula::Acl.new_with_id(acl.to_i, @client).delete
            }
        end

        # Delete ACLs 
        def delete_acls
            @vdc.acls.split(",").each{|acl|
                OpenNebula::Acl.new_with_id(acl.to_i, @client).delete
            }
        end

        # Deletes images 
        def delete_images
            ip = OpenNebula::ImagePool.new(@client)
            ip.info

            ip.each{|image|
                image.delete if image['GID'].to_i == @vdc.group_id
            }
        end

        # Deletes templates 
        def delete_templates
            tp = OpenNebula::TemplatePool.new(@client)
            tp.info

            tp.each{|template|
                template.delete if template['GID'].to_i == @vdc.group_id
            }
        end

        # Deletes VMs 
        def delete_vms
            vmp = OpenNebula::VirtualMachinePool.new(@client)
            vmp.info

            vmp.each{|vm|
                vm.delete if vm['GID'].to_i == @vdc.group_id
            }
        end

        # Deletes VNs 
        def delete_vns
            vnp = OpenNebula::VirtualNetworkPool.new(@client)
            vnp.info

            vnp.each{|vn|
                vnp.delete if vn['GID'].to_i == @vdc.group_id
            }
        end

        #######################################################################
        # Misc helper functions for the class
        #######################################################################

        # Deletes resources from failed created VDC
        def rollback(group, user, acls, rc)
            group.delete
            user.delete if user
           
            if acls
                acls.chop
                acls.split(",").each{|acl|
                    OpenNebula::Acl.new_with_id(acl.to_i, @client).delete
                }
            end 

            return rc
        end

        # Creates an acl array of acl strings. Returns true or error and
        # a comma-separated list with the new acl ids
        def create_acls(acls)
            acls_str = ""
            rc       = true

            acls.each{|rule|
                acl = OpenNebula::Acl.new(OpenNebula::Acl.build_xml,@client)
                rc  = acl.allocate(*OpenNebula::Acl.parse_rule(rule))

                break if OpenNebula.is_error?(rc)
                
                acls_str << acl.id.to_s << ","
            }

            return rc, acls_str.chop
        end
    end
end
