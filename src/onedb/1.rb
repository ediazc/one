# -------------------------------------------------------------------------- */
# Copyright 2002-2011, OpenNebula Project Leads (OpenNebula.org)             #
# Licensed under the Apache License, Version 2.0 (the "License"); you may    */
# not use this file except in compliance with the License. You may obtain    */
# a copy of the License at                                                   */
#                                                                            */
# http://www.apache.org/licenses/LICENSE-2.0                                 */
#                                                                            */
# Unless required by applicable law or agreed to in writing, software        */
# distributed under the License is distributed on an "AS IS" BASIS,          */
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */
# See the License for the specific language governing permissions and        */
# limitations under the License.                                             */
# -------------------------------------------------------------------------- */

class Migrator < MigratorBase

    def initialize(db, verbose)
        super(db, verbose)
        @db_version  = 1
        @one_version = "OpenNebula 2.3.0"
    end

    def up

        ########################################################################
        # Users
        ########################################################################

        # 2.2 Schema
        # CREATE TABLE user_pool (oid INTEGER PRIMARY KEY, user_name VARCHAR(256), password TEXT,enabled INTEGER, UNIQUE(user_name));

        # Move table user_pool
        @db.run "ALTER TABLE user_pool RENAME TO old_user_pool;"

        # Create new user_pool
        @db.run "CREATE TABLE user_pool (oid INTEGER PRIMARY KEY, name VARCHAR(256), body TEXT, UNIQUE(name));"

        user_group_ids = ""

        # Read each entry in the old user_pool, and insert into new user_pool
        @db.fetch("SELECT * FROM old_user_pool") do |row|
            oid  = row[:oid]

            if( oid == 0 )
                gid = 0
            else
                gid = 1
                user_group_ids += "<ID>#{oid}</ID>"
            end

            name = row[:user_name]

            body = "<USER><ID>#{oid}</ID><GID>#{gid}</GID><NAME>#{name}</NAME><PASSWORD>#{row[:password]}</PASSWORD><ENABLED>#{row[:enabled]}</ENABLED><GROUPS><ID>#{gid}</ID></GROUPS></USER>"

            @db.run "INSERT INTO user_pool VALUES(#{oid},'#{name}','#{body}');"
        end

        # Delete old user_pool
        @db.run "DROP TABLE old_user_pool"

        ########################################################################
        # Hosts
        ########################################################################

        # 2.2 Schema
        # CREATE TABLE host_pool (oid INTEGER PRIMARY KEY,host_name VARCHAR(256), state INTEGER,im_mad VARCHAR(128),vm_mad VARCHAR(128),tm_mad VARCHAR(128),last_mon_time INTEGER, cluster VARCHAR(128), template TEXT, UNIQUE(host_name));
        # CREATE TABLE host_shares(hid INTEGER PRIMARY KEY,disk_usage INTEGER, mem_usage INTEGER, cpu_usage INTEGER,max_disk  INTEGER,  max_mem   INTEGER, max_cpu   INTEGER,free_disk INTEGER,  free_mem  INTEGER, free_cpu  INTEGER,used_disk INTEGER,  used_mem  INTEGER, used_cpu  INTEGER,running_vms INTEGER);

        # Move table
        @db.run "ALTER TABLE host_pool RENAME TO old_host_pool;"

        # Create new table
        @db.run "CREATE TABLE host_pool (oid INTEGER PRIMARY KEY, name VARCHAR(256), body TEXT, state INTEGER, last_mon_time INTEGER, cid INTEGER, UNIQUE(name));"

        cluster_host_ids = Hash.new

        # Read each entry in the old table, and insert into new table
        @db.fetch("SELECT * FROM old_host_pool") do |row|
            oid             = row[:oid]
            name            = row[:host_name]
            state           = row[:state]
            last_mon_time   = row[:last_mon_time]
            cluster         = row[:cluster]

            # OpenNebula 2.X stored the cluster name, we need the cluster ID
            cluster_id = 0
            @db.fetch("SELECT oid FROM cluster_pool WHERE cluster_name='#{cluster}'") do |cluster_row|
                cluster_id = cluster_row[:oid]

                cluster_host_ids[cluster_id] = "" if cluster_host_ids[cluster_id] == nil
                cluster_host_ids[cluster_id] += "<ID>#{oid}</ID>"
            end

            # There is one host share for each host
            host_share = ""
            @db.fetch("SELECT * FROM host_shares WHERE hid=#{oid}") do |share|
                host_share = "<HOST_SHARE><DISK_USAGE>#{share[:disk_usage]}</DISK_USAGE><MEM_USAGE>#{share[:mem_usage]}</MEM_USAGE><CPU_USAGE>#{share[:cpu_usage]}</CPU_USAGE><MAX_DISK>#{share[:max_disk]}</MAX_DISK><MAX_MEM>#{share[:max_mem]}</MAX_MEM><MAX_CPU>#{share[:max_cpu]}</MAX_CPU><FREE_DISK>#{share[:free_disk]}</FREE_DISK><FREE_MEM>#{share[:free_mem]}</FREE_MEM><FREE_CPU>#{share[:free_cpu]}</FREE_CPU><USED_DISK>#{share[:used_disk]}</USED_DISK><USED_MEM>#{share[:used_mem]}</USED_MEM><USED_CPU>#{share[:used_cpu]}</USED_CPU><RUNNING_VMS>#{share[:running_vms]}</RUNNING_VMS></HOST_SHARE>"
            end

            body = "<HOST><ID>#{oid}</ID><NAME>#{name}</NAME><STATE>#{state}</STATE><IM_MAD>#{row[:im_mad]}</IM_MAD><VM_MAD>#{row[:vm_mad]}</VM_MAD><TM_MAD>#{row[:tm_mad]}</TM_MAD><LAST_MON_TIME>#{last_mon_time}</LAST_MON_TIME><CID>#{cluster_id}</CID>#{host_share}#{row[:template]}</HOST>"

            @db.run "INSERT INTO host_pool VALUES(#{oid},'#{name}','#{body}', #{state}, #{last_mon_time}, #{cluster_id});"
        end

        # Delete old table
        @db.run "DROP TABLE old_host_pool"
        @db.run "DROP TABLE host_shares"

        ########################################################################
        # Clusters
        ########################################################################

        # 2.2 Schema
        # CREATE TABLE cluster_pool (oid INTEGER PRIMARY KEY, cluster_name VARCHAR(128), UNIQUE(cluster_name) );

        # Move table
        @db.run "ALTER TABLE cluster_pool RENAME TO old_cluster_pool;"

        # Create new table
        @db.run "CREATE TABLE cluster_pool (oid INTEGER PRIMARY KEY, name VARCHAR(256), body TEXT, UNIQUE(name));"

        # Read each entry in the old table, and insert into new table
        @db.fetch("SELECT * FROM old_cluster_pool") do |row|
            oid  = row[:oid]
            name = row[:cluster_name]

            hids = ""
            if cluster_host_ids[oid] != nil
                hids = cluster_host_ids[oid]
            end

            body = "<CLUSTER><ID>#{oid}</ID><NAME>#{name}</NAME><HOSTS>#{hids}</HOSTS></CLUSTER>"

            @db.run "INSERT INTO cluster_pool VALUES(#{oid},'#{name}','#{body}');"
        end

        # Delete old table
        @db.run "DROP TABLE old_cluster_pool"

        ########################################################################
        # Images
        ########################################################################

        # 2.2 Schema
        # CREATE TABLE image_pool (oid INTEGER PRIMARY KEY, uid INTEGER, name VARCHAR(128), type INTEGER, public INTEGER, persistent INTEGER, regtime INTEGER, source TEXT, state INTEGER, running_vms INTEGER, template TEXT, UNIQUE(name) );

        # Move table
        @db.run "ALTER TABLE image_pool RENAME TO old_image_pool;"

        # Create new table
        @db.run "CREATE TABLE image_pool (oid INTEGER PRIMARY KEY, name VARCHAR(256), body TEXT, uid INTEGER, gid INTEGER, public INTEGER, UNIQUE(name,uid) );"

        # Read each entry in the old table, and insert into new table
        @db.fetch("SELECT * FROM old_image_pool") do |row|
            oid    = row[:oid]
            name   = row[:name]
            uid    = row[:uid]
            gid    = (uid == 0) ? 0 : 1
            public = row[:public]

            # In OpenNebula 2.0 Image States go from 0 to 3, in 3.0 go
            # from 0 to 5, but the meaning is the same for states 0 to 3
            body = "<IMAGE><ID>#{oid}</ID><UID>#{row[:uid]}</UID><GID>#{gid}</GID><NAME>#{name}</NAME><TYPE>#{row[:type]}</TYPE><PUBLIC>#{public}</PUBLIC><PERSISTENT>#{row[:persistent]}</PERSISTENT><REGTIME>#{row[:regtime]}</REGTIME><SOURCE>#{row[:source]}</SOURCE><STATE>#{row[:state]}</STATE><RUNNING_VMS>#{row[:running_vms]}</RUNNING_VMS>#{row[:template]}</IMAGE>"

            @db.run "INSERT INTO image_pool VALUES(#{oid},'#{name}','#{body}', #{uid}, #{gid}, #{public});"
        end

        # Delete old table
        @db.run "DROP TABLE old_image_pool"

        ########################################################################
        # VMs
        ########################################################################

        # 2.2 Schema
        # CREATE TABLE vm_pool (oid INTEGER PRIMARY KEY,uid INTEGER,name TEXT,last_poll INTEGER, state INTEGER,lcm_state INTEGER,stime INTEGER,etime INTEGER,deploy_id TEXT,memory INTEGER,cpu INTEGER,net_tx INTEGER,net_rx INTEGER, last_seq INTEGER, template TEXT);
        # CREATE TABLE history (vid INTEGER,seq INTEGER,host_name TEXT,vm_dir TEXT,hid INTEGER,vm_mad TEXT,tm_mad TEXT,stime INTEGER,etime INTEGER,pstime INTEGER,petime INTEGER,rstime INTEGER,retime INTEGER,estime INTEGER,eetime INTEGER,reason INTEGER,PRIMARY KEY(vid,seq));

        # Move tables
        @db.run "ALTER TABLE vm_pool RENAME TO old_vm_pool;"
        @db.run "ALTER TABLE history RENAME TO old_history;"

        # Create new tables
        @db.run "CREATE TABLE vm_pool (oid INTEGER PRIMARY KEY, name TEXT, body TEXT, uid INTEGER, gid INTEGER, last_poll INTEGER, state INTEGER, lcm_state INTEGER);"
        @db.run "CREATE TABLE history (vid INTEGER, seq INTEGER, body TEXT, PRIMARY KEY(vid,seq));"


        # Read each entry in the old history table, and insert into new table
        @db.fetch("SELECT * FROM old_history") do |row|
            vid = row[:vid]
            seq = row[:seq]

            body = "<HISTORY><SEQ>#{seq}</SEQ><HOSTNAME>#{row[:host_name]}</HOSTNAME><VM_DIR>#{row[:vm_dir]}</VM_DIR><HID>#{row[:hid]}</HID><STIME>#{row[:stime]}</STIME><ETIME>#{row[:etime]}</ETIME><VMMMAD>#{row[:vm_mad]}</VMMMAD><TMMAD>#{row[:tm_mad]}</TMMAD><PSTIME>#{row[:pstime]}</PSTIME><PETIME>#{row[:petime]}</PETIME><RSTIME>#{row[:rstime]}</RSTIME><RETIME>#{row[:retime]}</RETIME><ESTIME>#{row[:estime]}</ESTIME><EETIME>#{row[:eetime]}</EETIME><REASON>#{row[:reason]}</REASON></HISTORY>"

            @db.run "INSERT INTO history VALUES(#{vid},'#{seq}','#{body}');"
        end


        # Read each entry in the old vm table, and insert into new table
        @db.fetch("SELECT * FROM old_vm_pool") do |row|
            oid       = row[:oid]
            name      = row[:name]
            uid       = row[:uid]
            gid       = (uid == 0) ? 0 : 1
            last_poll = row[:last_poll]
            state     = row[:state]
            lcm_state = row[:lcm_state]

            # If the VM has History items, the last one is included in the XML
            history = ""
            @db.fetch("SELECT body FROM history WHERE vid=#{oid} AND seq=(SELECT MAX(seq) FROM history WHERE vid=#{oid})") do |history_row|
                history = history_row[:body]
            end

            body = "<VM><ID>#{oid}</ID><UID>#{uid}</UID><GID>#{gid}</GID><NAME>#{name}</NAME><LAST_POLL>#{last_poll}</LAST_POLL><STATE>#{state}</STATE><LCM_STATE>#{lcm_state}</LCM_STATE><STIME>#{row[:stime]}</STIME><ETIME>#{row[:etime]}</ETIME><DEPLOY_ID>#{row[:deploy_id]}</DEPLOY_ID><MEMORY>#{row[:memory]}</MEMORY><CPU>#{row[:cpu]}</CPU><NET_TX>#{row[:net_tx]}</NET_TX><NET_RX>#{row[:net_rx]}</NET_RX>#{row[:template]}#{history}</VM>"

            @db.run "INSERT INTO vm_pool VALUES(#{oid},'#{name}','#{body}', #{uid}, #{gid}, #{last_poll}, #{state}, #{lcm_state});"
        end


        # Delete old tables
        @db.run "DROP TABLE old_vm_pool"
        @db.run "DROP TABLE old_history"


        ########################################################################
        # Virtual Networks
        ########################################################################

        # 2.2 Schema
        # CREATE TABLE network_pool (oid INTEGER PRIMARY KEY, uid INTEGER, name VARCHAR(256), type INTEGER, bridge TEXT, public INTEGER, template TEXT, UNIQUE(name));
        # CREATE TABLE leases (oid INTEGER, ip BIGINT, mac_prefix BIGINT, mac_suffix BIGINT,vid INTEGER, used INTEGER, PRIMARY KEY(oid,ip));

        # Move tables
        @db.run "ALTER TABLE network_pool RENAME TO old_network_pool;"
        @db.run "ALTER TABLE leases RENAME TO old_leases;"

        # Create new tables
        @db.run "CREATE TABLE network_pool (oid INTEGER PRIMARY KEY, name VARCHAR(128), body TEXT, uid INTEGER, gid INTEGER, public INTEGER, UNIQUE(name,uid));"
        @db.run "CREATE TABLE leases (oid INTEGER, ip BIGINT, body TEXT, PRIMARY KEY(oid,ip));"

        # Read each entry in the old table, and insert into new table
        @db.fetch("SELECT * FROM old_network_pool") do |row|
            oid    = row[:oid]
            name   = row[:name]
            uid    = row[:uid]
            gid    = (uid == 0) ? 0 : 1
            public = row[:public]

            # <TOTAL_LEASES> is stored in the DB, but it is not used to rebuild
            # the VirtualNetwork object, and it is generated each time the
            # network is listed. So setting it to 0 is safe
            body = "<VNET><ID>#{oid}</ID><UID>#{uid}</UID><GID>#{gid}</GID><NAME>#{name}</NAME><TYPE>#{row[:type]}</TYPE><BRIDGE>#{row[:bridge]}</BRIDGE><PUBLIC>#{public}</PUBLIC><TOTAL_LEASES>0</TOTAL_LEASES>#{row[:template]}</VNET>"

            @db.run "INSERT INTO network_pool VALUES(#{oid},'#{name}','#{body}', #{uid}, #{gid}, #{public});"
        end

        # Read each entry in the old table, and insert into new table
        @db.fetch("SELECT * FROM old_leases") do |row|
            oid = row[:oid]
            ip  = row[:ip]

            body = "<LEASE><IP>#{ip}</IP><MAC_PREFIX>#{row[:mac_prefix]}</MAC_PREFIX><MAC_SUFFIX>#{row[:mac_suffix]}</MAC_SUFFIX><USED>#{row[:used]}</USED><VID>#{row[:vid]}</VID></LEASE>"

            @db.run "INSERT INTO leases VALUES(#{oid}, #{ip}, '#{body}');"
        end

        # Delete old tables
        @db.run "DROP TABLE old_network_pool"
        @db.run "DROP TABLE old_leases"


        ########################################################################
        # New tables in DB version 1
        ########################################################################

        @db.run "CREATE TABLE db_versioning (oid INTEGER PRIMARY KEY, version INTEGER, timestamp INTEGER, comment VARCHAR(256));"
        @db.run "CREATE TABLE template_pool (oid INTEGER PRIMARY KEY, name VARCHAR(256), body TEXT, uid INTEGER, gid INTEGER, public INTEGER);"

        # The group pool has two default ones
        @db.run "CREATE TABLE group_pool (oid INTEGER PRIMARY KEY, name VARCHAR(256), body TEXT, uid INTEGER, UNIQUE(name));"
        @db.run "INSERT INTO group_pool VALUES(0,'oneadmin','<GROUP><ID>0</ID><UID>0</UID><NAME>oneadmin</NAME><USERS><ID>0</ID></USERS></GROUP>',0);"
        @db.run "INSERT INTO group_pool VALUES(1,'users','<GROUP><ID>1</ID><UID>0</UID><NAME>users</NAME><USERS>#{user_group_ids}</USERS></GROUP>',0);"

        # New pool_control table contains the last_oid used, must be rebuilt
        @db.run "CREATE TABLE pool_control (tablename VARCHAR(32) PRIMARY KEY, last_oid BIGINT UNSIGNED)"

        for table in ["user_pool", "cluster_pool", "host_pool", "image_pool", "vm_pool", "network_pool"] do
            @db.fetch("SELECT MAX(oid) FROM #{table}") do |row|
                if( row[:"MAX(oid)"] != nil )
                    @db.run "INSERT INTO pool_control (tablename, last_oid) VALUES ('#{table}', #{row[:"MAX(oid)"]});"
                end
            end
        end

        # First 100 group Ids are reserved for system groups.
        # Regular ones start from ID 100
        @db.run "INSERT INTO pool_control (tablename, last_oid) VALUES ('group_pool', 99);"

        return true
    end
end
