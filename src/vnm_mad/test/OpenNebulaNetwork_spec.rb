#!/usr/bin/env ruby

$: << File.dirname(__FILE__) + '/..' \
   << './'

require 'rubygems'
require 'rspec'
require 'SystemMock'
require 'pp'

require 'OpenNebulaNetwork'
require 'Ebtables'
require 'Firewall'
require 'HostManaged'
require 'OpenvSwitch'

OUTPUT = Hash.new
Dir[File.dirname(__FILE__) + "/output/**"].each do |f|
    key = File.basename(f).to_sym
    OUTPUT[key] = File.read(f)
end

include SystemMock

RSpec.configure do |config|
    config.before(:each) do
        $capture_commands = Hash.new
        $collector = Hash.new
    end
end

describe 'networking' do
    it "get all nics in kvm" do
        $capture_commands = {
            /virsh.*dumpxml/ => OUTPUT[:virsh_dumpxml]
        }
        onevlan = OpenNebulaNetwork.new(OUTPUT[:onevm_show],"kvm")
        nics_expected = [{:bridge=>"br0",
                      :ip=>"172.16.0.100",
                      :mac=>"02:00:ac:10:00:64",
                      :network=>"Small network",
                      :network_id=>"0",
                      :tap=>"vnet0"},
                     {:bridge=>"br1",
                      :ip=>"10.1.1.1",
                      :mac=>"02:00:0a:01:01:01",
                      :network=>"r1",
                      :network_id=>"1",
                      :tap=>"vnet1"},
                     {:bridge=>"br2",
                      :ip=>"10.1.2.1",
                      :mac=>"02:00:0a:01:02:01",
                      :network=>"r2",
                      :network_id=>"2",
                      :tap=>"vnet2"}]
        onevlan.vm.nics.should == nics_expected
    end

    it "filter nics in kvm" do
        $capture_commands = {
            /virsh.*dumpxml/ => OUTPUT[:virsh_dumpxml]
        }
        onevlan = OpenNebulaNetwork.new(OUTPUT[:onevm_show],"kvm")
        onevlan.filter(:bridge => "br1")
        nics_expected = [{:bridge=>"br1",
                          :ip=>"10.1.1.1",
                          :mac=>"02:00:0a:01:01:01",
                          :network=>"r1",
                          :network_id=>"1",
                          :tap=>"vnet1"}]
 
        onevlan.vm.filtered_nics.should == nics_expected
    end
end

describe 'ebtables' do
    it "generate ebtable rules in kvm" do
        $capture_commands = {
            /virsh.*dumpxml/ => OUTPUT[:virsh_dumpxml],
            /ebtables/       => nil
        }
        onevlan = EbtablesVLAN.new(OUTPUT[:onevm_show],"kvm")
        onevlan.activate
        ebtables_cmds = [
        "sudo /sbin/ebtables -A FORWARD -s ! 02:00:ac:10:00:00/ff:ff:ff:ff:ff:00 -o vnet0 -j DROP",
        "sudo /sbin/ebtables -A FORWARD -s ! 02:00:ac:10:00:64 -i vnet0 -j DROP",
        "sudo /sbin/ebtables -A FORWARD -s ! 02:00:0a:01:01:00/ff:ff:ff:ff:ff:00 -o vnet1 -j DROP",
        "sudo /sbin/ebtables -A FORWARD -s ! 02:00:0a:01:01:01 -i vnet1 -j DROP",
        "sudo /sbin/ebtables -A FORWARD -s ! 02:00:0a:01:02:00/ff:ff:ff:ff:ff:00 -o vnet2 -j DROP",
        "sudo /sbin/ebtables -A FORWARD -s ! 02:00:0a:01:02:01 -i vnet2 -j DROP"]
        $collector[:system].should == ebtables_cmds
    end
end

describe 'openvswitch' do
    it "tag tun/tap devices with vlans in kvm" do
        $capture_commands = {
            /virsh.*dumpxml/ => OUTPUT[:virsh_dumpxml],
            /ovs-vsctl/      => nil
        }
        onevlan = OpenvSwitchVLAN.new(OUTPUT[:onevm_show],"kvm")
        onevlan.activate
        openvswitch_tags = [
            "sudo /usr/local/bin/ovs-vsctl set Port vnet0 tag=2",
            "sudo /usr/local/bin/ovs-vsctl set Port vnet1 tag=3",
            "sudo /usr/local/bin/ovs-vsctl set Port vnet2 tag=4"
            ]

        $collector[:system].should == openvswitch_tags
    end

    it "force VLAN_ID for Open vSwitch vlans in kvm" do
        $capture_commands = {
            /virsh.*dumpxml/ => OUTPUT[:virsh_dumpxml_vlan_id],
            /brctl show/     => OUTPUT[:brctl_show],
            /ovs-vsctl/      => nil
        }
        onevlan = OpenvSwitchVLAN.new(OUTPUT[:onevm_show_vlan_id_kvm],"kvm")
        onevlan.activate
        
        onevlan_rules = ["sudo /usr/local/bin/ovs-vsctl set Port vnet0 tag=6",
                         "sudo /usr/local/bin/ovs-vsctl set Port vnet1 tag=50",
                         "sudo /usr/local/bin/ovs-vsctl set Port vnet1 tag=51"]

        $collector[:system].should == onevlan_rules
    end
end

describe 'firewall' do
    it "should activate FW rules in xen" do
        $capture_commands = {
            /uname/ => OUTPUT[:xen_uname_a],
            /lsmod/ => OUTPUT[:xen_lsmod],
            /network-list/ => OUTPUT[:xm_network_list],
            /domid/ => OUTPUT[:xm_domid],
            /iptables/ => nil
        }
        fw = OpenNebulaFirewall.new(OUTPUT[:onevm_show_xen])
        fw.activate

        fw_activate_rules = ["sudo /sbin/iptables -N one-36-3",
                 "sudo /sbin/iptables -A FORWARD -m physdev --physdev-out vif4.0 -j one-36-3",
                 "sudo /sbin/iptables -A one-36-3 -p tcp -m state --state ESTABLISHED -j ACCEPT",
                 "sudo /sbin/iptables -A one-36-3 -p tcp -m multiport --dports 22,80 -j ACCEPT",
                 "sudo /sbin/iptables -A one-36-3 -p tcp -j DROP",
                 "sudo /sbin/iptables -A one-36-3 -p icmp -m state --state ESTABLISHED -j ACCEPT",
                 "sudo /sbin/iptables -A one-36-3 -p icmp -j DROP"]

        $collector[:system].should == fw_activate_rules
    end
end

describe 'host-managed' do
    it "tag tun/tap devices with vlans in kvm" do
        $capture_commands = {
            /virsh.*dumpxml/ => OUTPUT[:virsh_dumpxml_phydev],
            /brctl show/     => OUTPUT[:brctl_show],
	    /brctl add/    => nil,
	    /vconfig/        => nil,
	    /ip link/        => nil
        }
        hm = OpenNebulaHM.new(OUTPUT[:onevm_show_phydev_kvm],"kvm")
        hm.activate

        hm_activate_rules = ["sudo /usr/sbin/brctl addbr onebr6",
                             "sudo /sbin/ip link set onebr6 up",
                             "sudo /sbin/ip link show eth0.8",
                             "sudo /sbin/vconfig add eth0 8",
                             "sudo /sbin/ip link set eth0.8 up",
                             "sudo /usr/sbin/brctl addif onebr6 eth0.8"]
        $collector[:system].should == hm_activate_rules
    end

    it "force VLAN_ID for vlans in kvm" do
        $capture_commands = {
            /virsh.*dumpxml/ => OUTPUT[:virsh_dumpxml_vlan_id],
            /brctl show/     => OUTPUT[:brctl_show],
            /brctl add/      => nil,
            /vconfig/        => nil,
            /ip link/        => nil
        }
        hm = OpenNebulaHM.new(OUTPUT[:onevm_show_vlan_id_kvm],"kvm")
        hm.activate

        hm_vlan_id = ["sudo /usr/sbin/brctl addbr onebr10",
                      "sudo /sbin/ip link set onebr10 up",
                      "sudo /sbin/ip link show eth0.50",
                      "sudo /sbin/vconfig add eth0 50",
                      "sudo /sbin/ip link set eth0.50 up",
                      "sudo /usr/sbin/brctl addif onebr10 eth0.50",
                      "sudo /usr/sbin/brctl addbr specialbr",
                      "sudo /sbin/ip link set specialbr up",
                      "sudo /sbin/ip link show eth0.51",
                      "sudo /sbin/vconfig add eth0 51",
                      "sudo /sbin/ip link set eth0.51 up",
                      "sudo /usr/sbin/brctl addif specialbr eth0.51"]

        $collector[:system].should == hm_vlan_id
    end
end
