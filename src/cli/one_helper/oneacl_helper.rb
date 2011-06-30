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

require 'one_helper'

class OneAclHelper < OpenNebulaHelper::OneHelper
    def self.rname
        "ACL"
    end

    def self.conf_file
        "oneacl.yaml"
    end
    
    def add_rule(options, arg0, arg1=nil, arg2=nil)
        aclp = OpenNebula::AclPool.new( OpenNebula::Client.new() )

        if arg2
            rc = aclp.addrule( arg0, arg1, arg2 )
        else
            rc = aclp.addrule_with_str( arg0 )
        end  
        
        if OpenNebula.is_error?(rc)
            [-1, rc.message]
        else
            if !rc
                puts "Rule added" if options[:verbose]
                return 0
            end
            return [-1, rc[:users].message] if OpenNebula.is_error?(rc[:users])
            return [-1, rc[:resources].message] if OpenNebula.is_error?(
                                                                 rc[:resources])
            return [-1, rc[:rights].message] if OpenNebula.is_error?(
                                                                 rc[:rights])
        end
    end
    
    def delete_rule(id) 
        acl = OpenNebula::AclPool.new( OpenNebula::Client.new() )

        rc = acl.delrule( id )

        if OpenNebula.is_error?(rc)
            [-1, rc.message]
        else
            puts "Rule deleted" if options[:verbose]
            0
        end
    end

    private

    def factory_pool(filter)
        OpenNebula::AclPool.new(@client)
    end
    
        # TODO check that @content[:resources_str]  is valid
    def self.resource_mask(str)
        resource_type=str.split("/")[0]
  
        mask = "-------"
             
        resource_type.split("+").each{|type|
            case type
                when "VM"
                    mask[0] = "V"   
                when "HOST"
                    mask[1] = "H"   
                when "NET"
                    mask[2] = "N"  
                when "IMAGE"
                    mask[3] = "I"   
                when "USER"
                    mask[4] = "U"
                when "TEMPLATE"
                    mask[5] = "T"
                when "GROUP"
                    mask[6] = "G"           
            end     
        }            
        mask
    end
    
    # TODO check that @content[:resources_str]  is valid
    def self.right_mask(str)
        mask = "---------"  
        
        str.split("+").each{|type|
            case type
                when "CREATE"
                    mask[0] = "C"   
                when "DELETE"
                    mask[1] = "D"   
                when "USE"
                    mask[2] = "U"  
                when "MANAGE"
                    mask[3] = "M"   
                when "INFO"
                    mask[4] = "I"
                when "INFO_POOL"
                    mask[5] = "P"
                when "INFO_POOL_MINE"
                    mask[6] = "p"           
                when "INSTANTIATE"
                    mask[8] = "T"  
                when "CHOWN"
                    mask[9] = "W"  
            end                                                                                                                                                                                                                                                                                                                  
        }
        
        mask
    end

    def format_pool(pool, options, top=false)
        config_file=self.class.table_conf

        table=CLIHelper::ShowTable.new(config_file, self) do
            column :ID, "Rule Identifier", 
                          :size=>5 do |d|            
                d['ID']
            end
            
            column :USER, "To which resource owner the rule applies to", 
                          :size=>8 do |d|            
                d['STRING'].split(" ")[0]
            end
            
            column :RES_VHNIUTG, "Resource to which the rule applies" do |d|
               OneAclHelper::resource_mask d['STRING'].split(" ")[1] 
            end
            
            column :RID, "Resource ID", :right, :size=>8 do |d|
                d['STRING'].split(" ")[1].split("/")[1]
            end
                  
            column :OPE_CDUMIPpTW, "Operation to which the rule applies" do |d|
                OneAclHelper::right_mask d['STRING'].split(" ")[2] 
            end

            default :ID, :USER, :RESOURCE_VHNIUTG, :RID, :OPERATION_CDUMIPpTW
        end

        table.show(pool, options)
        
    end
    
end