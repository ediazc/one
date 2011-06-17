#!/bin/bash

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

#-------------------------------------------------------------------------------
# Install program for OpenNebula. It will install it relative to
# $ONE_LOCATION if defined with the -d option, otherwise it'll be installed
# under /. In this case you may specified the oneadmin user/group, so you do
# not need run the OpenNebula daemon with root priviledges
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# COMMAND LINE PARSING
#-------------------------------------------------------------------------------
usage() {
 echo
 echo "Usage: install.sh [-u install_user] [-g install_group] [-k keep conf]"
 echo "                  [-d ONE_LOCATION] [-c occi|ec2] [-r] [-h]"
 echo
 echo "-u: user that will run opennebula, defaults to user executing install.sh"
 echo "-g: group of the user that will run opennebula, defaults to user"
 echo "    executing install.sh"
 echo "-k: keep configuration files of existing OpenNebula installation, useful"
 echo "    when upgrading. This flag should not be set when installing"
 echo "    OpenNebula for the first time"
 echo "-d: target installation directory, if not defined it'd be root. Must be"
 echo "    an absolute path."
 echo "-c: install client utilities: OpenNebula cli, occi and ec2 client files"
 echo "-s: install OpenNebula Sunstone"
 echo "-o: install OpenNebula Zones (OZones)"
 echo "-r: remove Opennebula, only useful if -d was not specified, otherwise"
 echo "    rm -rf \$ONE_LOCATION would do the job"
 echo "-l: creates symlinks instead of copying files, useful for development"
 echo "-h: prints this help"
}
#-------------------------------------------------------------------------------

TEMP_OPT=`getopt -o hkrlcsou:g:d: -n 'install.sh' -- "$@"`

if [ $? != 0 ] ; then
    usage
    exit 1
fi

eval set -- "$TEMP_OPT"

INSTALL_ETC="yes"
UNINSTALL="no"
LINK="no"
CLIENT="no"
SUNSTONE="no"
OZONES="no"
ONEADMIN_USER=`id -u`
ONEADMIN_GROUP=`id -g`
SRC_DIR=$PWD

while true ; do
    case "$1" in
        -h) usage; exit 0;;
        -k) INSTALL_ETC="no"   ; shift ;;
        -r) UNINSTALL="yes"   ; shift ;;
        -l) LINK="yes" ; shift ;;
        -c) CLIENT="yes"; INSTALL_ETC="no" ; shift ;;
        -s) SUNSTONE="yes"; INSTALL_ETC="no" ; shift ;;
        -o) OZONES="yes"; INSTALL_ETC="no" ; shift ;;
        -u) ONEADMIN_USER="$2" ; shift 2;;
        -g) ONEADMIN_GROUP="$2"; shift 2;;
        -d) ROOT="$2" ; shift 2 ;;
        --) shift ; break ;;
        *)  usage; exit 1 ;;
    esac
done

#-------------------------------------------------------------------------------
# Definition of locations
#-------------------------------------------------------------------------------

if [ -z "$ROOT" ] ; then
    BIN_LOCATION="/usr/bin"
    LIB_LOCATION="/usr/lib/one"
    ETC_LOCATION="/etc/one"
    LOG_LOCATION="/var/log/one"
    VAR_LOCATION="/var/lib/one"
    SUNSTONE_LOCATION="$LIB_LOCATION/sunstone"
    OZONES_LOCATION="$LIB_LOCATION/ozones"
    IMAGES_LOCATION="$VAR_LOCATION/images"
    RUN_LOCATION="/var/run/one"
    LOCK_LOCATION="/var/lock/one"
    INCLUDE_LOCATION="/usr/include"
    SHARE_LOCATION="/usr/share/one"
    MAN_LOCATION="/usr/share/man/man1"

    if [ "$CLIENT" = "yes" ]; then
        MAKE_DIRS="$BIN_LOCATION $LIB_LOCATION"

        DELETE_DIRS=""

        CHOWN_DIRS=""
    elif [ "$SUNSTONE" = "yes" ]; then
        MAKE_DIRS="$BIN_LOCATION $LIB_LOCATION $VAR_LOCATION $SUNSTONE_LOCATION"

        DELETE_DIRS="$MAKE_DIRS"

        CHOWN_DIRS=""
    elif [ "$OZONES" = "yes" ]; then
        MAKE_DIRS="$BIN_LOCATION $LIB_LOCATION $VAR_LOCATION $OZONES_LOCATION"

        DELETE_DIRS="$MAKE_DIRS"

        CHOWN_DIRS=""
    else
        MAKE_DIRS="$BIN_LOCATION $LIB_LOCATION $ETC_LOCATION $VAR_LOCATION \
                   $INCLUDE_LOCATION $SHARE_LOCATION \
                   $LOG_LOCATION $RUN_LOCATION $LOCK_LOCATION \
                   $IMAGES_LOCATION $MAN_LOCATION"

        DELETE_DIRS="$LIB_LOCATION $ETC_LOCATION $LOG_LOCATION $VAR_LOCATION \
                     $RUN_LOCATION $SHARE_DIRS"

        CHOWN_DIRS="$LOG_LOCATION $VAR_LOCATION $RUN_LOCATION $LOCK_LOCATION"
    fi

else
    BIN_LOCATION="$ROOT/bin"
    LIB_LOCATION="$ROOT/lib"
    ETC_LOCATION="$ROOT/etc"
    VAR_LOCATION="$ROOT/var"
    SUNSTONE_LOCATION="$LIB_LOCATION/sunstone"
    OZONES_LOCATION="$LIB_LOCATION/ozones"
    IMAGES_LOCATION="$VAR_LOCATION/images"
    INCLUDE_LOCATION="$ROOT/include"
    SHARE_LOCATION="$ROOT/share"
    MAN_LOCATION="$ROOT/share/man/man1"

    if [ "$CLIENT" = "yes" ]; then
        MAKE_DIRS="$BIN_LOCATION $LIB_LOCATION"

        DELETE_DIRS="$MAKE_DIRS"
    elif [ "$SUNSTONE" = "yes" ]; then
        MAKE_DIRS="$BIN_LOCATION $LIB_LOCATION $VAR_LOCATION $SUNSTONE_LOCATION"

        DELETE_DIRS="$MAKE_DIRS"
    elif [ "$OZONES" = "yes" ]; then
        MAKE_DIRS="$BIN_LOCATION $LIB_LOCATION $VAR_LOCATION $OZONES_LOCATION"

        DELETE_DIRS="$MAKE_DIRS"        
    else
        MAKE_DIRS="$BIN_LOCATION $LIB_LOCATION $ETC_LOCATION $VAR_LOCATION \
                   $INCLUDE_LOCATION $SHARE_LOCATION $IMAGES_LOCATION \
                   $MAN_LOCATION"

        DELETE_DIRS="$MAKE_DIRS"

        CHOWN_DIRS="$ROOT"
    fi

    CHOWN_DIRS="$ROOT"
fi

SHARE_DIRS="$SHARE_LOCATION/examples \
            $SHARE_LOCATION/examples/tm \
            $SHARE_LOCATION/hooks"

ETC_DIRS="$ETC_LOCATION/im_kvm \
          $ETC_LOCATION/im_xen \
          $ETC_LOCATION/im_ec2 \
          $ETC_LOCATION/vmm_ec2 \
          $ETC_LOCATION/vmm_ssh \
          $ETC_LOCATION/vmm_sh \
          $ETC_LOCATION/tm_nfs \
          $ETC_LOCATION/tm_ssh \
          $ETC_LOCATION/tm_dummy \
          $ETC_LOCATION/tm_lvm \
          $ETC_LOCATION/hm \
          $ETC_LOCATION/auth \
          $ETC_LOCATION/ec2query_templates \
          $ETC_LOCATION/occi_templates"

LIB_DIRS="$LIB_LOCATION/ruby \
          $LIB_LOCATION/ruby/OpenNebula \
          $LIB_LOCATION/ruby/cloud/ \
          $LIB_LOCATION/ruby/cloud/econe \
          $LIB_LOCATION/ruby/cloud/econe/views \
          $LIB_LOCATION/ruby/cloud/occi \
          $LIB_LOCATION/onedb \
          $LIB_LOCATION/tm_commands \
          $LIB_LOCATION/tm_commands/nfs \
          $LIB_LOCATION/tm_commands/ssh \
          $LIB_LOCATION/tm_commands/dummy \
          $LIB_LOCATION/tm_commands/lvm \
          $LIB_LOCATION/mads \
          $LIB_LOCATION/sh"

VAR_DIRS="$VAR_LOCATION/remotes \
          $VAR_LOCATION/remotes/im \
          $VAR_LOCATION/remotes/im/kvm.d \
          $VAR_LOCATION/remotes/im/xen.d \
          $VAR_LOCATION/remotes/im/ganglia.d \
          $VAR_LOCATION/remotes/vmm/xen \
          $VAR_LOCATION/remotes/vmm/kvm \
          $VAR_LOCATION/remotes/image \
          $VAR_LOCATION/remotes/image/fs"

SUNSTONE_DIRS="$SUNSTONE_LOCATION/models \
               $SUNSTONE_LOCATION/models/OpenNebulaJSON \
               $SUNSTONE_LOCATION/public \
               $SUNSTONE_LOCATION/public/js \
               $SUNSTONE_LOCATION/public/js/plugins \
               $SUNSTONE_LOCATION/public/css \
               $SUNSTONE_LOCATION/public/vendor \
               $SUNSTONE_LOCATION/public/vendor/jQueryLayout \
               $SUNSTONE_LOCATION/public/vendor/dataTables \
               $SUNSTONE_LOCATION/public/vendor/jQueryUI \
               $SUNSTONE_LOCATION/public/vendor/jQuery \
               $SUNSTONE_LOCATION/public/vendor/jGrowl \
               $SUNSTONE_LOCATION/public/images \
               $SUNSTONE_LOCATION/templates"
               
OZONES_DIRS="$OZONES_LOCATION/Server \
             $OZONES_LOCATION/Server/lib \
             $OZONES_LOCATION/Server/lib/OZones \
             $OZONES_LOCATION/Server/models"

OZONES_CLIENT_DIRS="$LIB_LOCATION/ruby \
                 $LIB_LOCATION/ruby/OpenNebula \
                 $OZONES_LOCATION/Client \
                 $OZONES_LOCATION/Client/lib \
                 $OZONES_LOCATION/Client/lib/CLI \
                 $OZONES_LOCATION/Client/lib/CLI/OZonesHelper"

LIB_ECO_CLIENT_DIRS="$LIB_LOCATION/ruby \
                 $LIB_LOCATION/ruby/OpenNebula \
                 $LIB_LOCATION/ruby/cloud/ \
                 $LIB_LOCATION/ruby/cloud/econe"

LIB_OCCI_CLIENT_DIRS="$LIB_LOCATION/ruby \
                 $LIB_LOCATION/ruby/OpenNebula \
                 $LIB_LOCATION/ruby/cloud/occi"

LIB_OCA_CLIENT_DIRS="$LIB_LOCATION/ruby \
                 $LIB_LOCATION/ruby/OpenNebula"

LIB_CLI_DIRS="$LIB_LOCATION/ruby \
              $LIB_LOCATION/ruby/OpenNebula"

if [ "$CLIENT" = "yes" ]; then
    MAKE_DIRS="$MAKE_DIRS $LIB_ECO_CLIENT_DIRS $LIB_OCCI_CLIENT_DIRS \
               $LIB_CLI_DIRS $OZONES_CLIENT_DIRS"
elif [ "$SUNSTONE" = "yes" ]; then
    MAKE_DIRS="$MAKE_DIRS $SUNSTONE_DIRS $LIB_OCA_CLIENT_DIRS"
elif [ "$OZONES" = "yes" ]; then
    MAKE_DIRS="$MAKE_DIRS $OZONES_DIRS $OZONES_CLIENT_DIRS $LIB_OCA_CLIENT_DIRS"
else
    MAKE_DIRS="$MAKE_DIRS $SHARE_DIRS $ETC_DIRS $LIB_DIRS $VAR_DIRS \
               $OZONES_DIRS $OZONES_CLIENT_DIRS $SUNSTONE_DIRS"
fi

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
# FILE DEFINITION, WHAT IS GOING TO BE INSTALLED AND WHERE
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
INSTALL_FILES=(
    BIN_FILES:$BIN_LOCATION
    INCLUDE_FILES:$INCLUDE_LOCATION
    LIB_FILES:$LIB_LOCATION
    RUBY_LIB_FILES:$LIB_LOCATION/ruby
    RUBY_OPENNEBULA_LIB_FILES:$LIB_LOCATION/ruby/OpenNebula
    MAD_RUBY_LIB_FILES:$LIB_LOCATION/ruby
    MAD_RUBY_LIB_FILES:$VAR_LOCATION/remotes
    MAD_SH_LIB_FILES:$LIB_LOCATION/sh
    MAD_SH_LIB_FILES:$VAR_LOCATION/remotes
    ONEDB_MIGRATOR_FILES:$LIB_LOCATION/onedb
    MADS_LIB_FILES:$LIB_LOCATION/mads
    IM_PROBES_FILES:$VAR_LOCATION/remotes/im
    IM_PROBES_KVM_FILES:$VAR_LOCATION/remotes/im/kvm.d
    IM_PROBES_XEN_FILES:$VAR_LOCATION/remotes/im/xen.d
    IM_PROBES_GANGLIA_FILES:$VAR_LOCATION/remotes/im/ganglia.d
    VMM_SSH_KVM_SCRIPTS:$VAR_LOCATION/remotes/vmm/kvm
    VMM_SSH_XEN_SCRIPTS:$VAR_LOCATION/remotes/vmm/xen
    VMM_SSH_XEN_KVM_POLL:$VAR_LOCATION/remotes/vmm/kvm/poll
    VMM_SSH_XEN_KVM_POLL:$VAR_LOCATION/remotes/vmm/xen/poll
    VMM_SSH_GANGLIA_POLL:$VAR_LOCATION/remotes/vmm/kvm/poll_local
    VMM_SSH_GANGLIA_POLL:$VAR_LOCATION/remotes/vmm/xen/poll_local
    NFS_TM_COMMANDS_LIB_FILES:$LIB_LOCATION/tm_commands/nfs
    SSH_TM_COMMANDS_LIB_FILES:$LIB_LOCATION/tm_commands/ssh
    DUMMY_TM_COMMANDS_LIB_FILES:$LIB_LOCATION/tm_commands/dummy
    LVM_TM_COMMANDS_LIB_FILES:$LIB_LOCATION/tm_commands/lvm
    IMAGE_DRIVER_FS_SCRIPTS:$VAR_LOCATION/remotes/image/fs
    EXAMPLE_SHARE_FILES:$SHARE_LOCATION/examples
    INSTALL_NOVNC_SHARE_FILE:$SHARE_LOCATION
    TM_EXAMPLE_SHARE_FILES:$SHARE_LOCATION/examples/tm
    HOOK_SHARE_FILES:$SHARE_LOCATION/hooks
    COMMON_CLOUD_LIB_FILES:$LIB_LOCATION/ruby/cloud
    ECO_LIB_FILES:$LIB_LOCATION/ruby/cloud/econe
    ECO_LIB_VIEW_FILES:$LIB_LOCATION/ruby/cloud/econe/views
    ECO_BIN_FILES:$BIN_LOCATION
    OCCI_LIB_FILES:$LIB_LOCATION/ruby/cloud/occi
    OCCI_BIN_FILES:$BIN_LOCATION
    MAN_FILES:$MAN_LOCATION
)

INSTALL_CLIENT_FILES=(
    COMMON_CLOUD_CLIENT_LIB_FILES:$LIB_LOCATION/ruby/cloud
    ECO_LIB_CLIENT_FILES:$LIB_LOCATION/ruby/cloud/econe
    ECO_BIN_CLIENT_FILES:$BIN_LOCATION
    COMMON_CLOUD_CLIENT_LIB_FILES:$LIB_LOCATION/ruby/cloud
    OCCI_LIB_CLIENT_FILES:$LIB_LOCATION/ruby/cloud/occi
    OCCI_BIN_CLIENT_FILES:$BIN_LOCATION
    OZONES_LIB_CLIENT_FILES:$OZONES_LOCATION/Client/lib
    OZONES_BIN_CLIENT_FILES:$BIN_LOCATION
    CLI_BIN_FILES:$BIN_LOCATION
    CLI_LIB_FILES:$LIB_LOCATION/ruby
    RUBY_OPENNEBULA_LIB_FILES:$LIB_LOCATION/ruby/OpenNebula
    OZONES_LIB_CLIENT_FILES:$OZONES_LOCATION/Client/lib
    OZONES_LIB_CLIENT_CLI_FILES:$OZONES_LOCATION/Client/lib/CLI
    OZONES_LIB_CLIENT_CLI_HELPER_FILES:$OZONES_LOCATION/Client/lib/CLI/OZonesHelper    
    OZONES_BIN_CLIENT_FILES:$BIN_LOCATION  
)

INSTALL_SUNSTONE_RUBY_FILES=(
    SUNSTONE_RUBY_LIB_FILES:$LIB_LOCATION/ruby
    RUBY_OPENNEBULA_LIB_FILES:$LIB_LOCATION/ruby/OpenNebula
)

INSTALL_SUNSTONE_FILES=(
    SUNSTONE_FILES:$SUNSTONE_LOCATION
    SUNSTONE_BIN_FILES:$BIN_LOCATION
    SUNSTONE_MODELS_FILES:$SUNSTONE_LOCATION/models
    SUNSTONE_MODELS_JSON_FILES:$SUNSTONE_LOCATION/models/OpenNebulaJSON
    SUNSTONE_TEMPLATE_FILES:$SUNSTONE_LOCATION/templates
    SUNSTONE_PUBLIC_JS_FILES:$SUNSTONE_LOCATION/public/js
    SUNSTONE_PUBLIC_JS_PLUGINS_FILES:$SUNSTONE_LOCATION/public/js/plugins
    SUNSTONE_PUBLIC_CSS_FILES:$SUNSTONE_LOCATION/public/css
    SUNSTONE_PUBLIC_VENDOR_DATATABLES:$SUNSTONE_LOCATION/public/vendor/dataTables
    SUNSTONE_PUBLIC_VENDOR_JGROWL:$SUNSTONE_LOCATION/public/vendor/jGrowl
    SUNSTONE_PUBLIC_VENDOR_JQUERY:$SUNSTONE_LOCATION/public/vendor/jQuery
    SUNSTONE_PUBLIC_VENDOR_JQUERYUI:$SUNSTONE_LOCATION/public/vendor/jQueryUI
    SUNSTONE_PUBLIC_VENDOR_JQUERYLAYOUT:$SUNSTONE_LOCATION/public/vendor/jQueryLayout
    SUNSTONE_PUBLIC_IMAGES_FILES:$SUNSTONE_LOCATION/public/images
)

INSTALL_OZONES_RUBY_FILES=(
    OZONES_RUBY_LIB_FILES:$LIB_LOCATION/ruby
    RUBY_OPENNEBULA_LIB_FILES:$LIB_LOCATION/ruby/OpenNebula
)

INSTALL_OZONES_FILES=(
    OZONES_FILES:$OZONES_LOCATION/Server
    OZONES_BIN_FILES:$BIN_LOCATION
    OZONES_MODELS_FILES:$OZONES_LOCATION/Server/models
    OZONES_LIB_FILES:$OZONES_LOCATION/Server/lib
    OZONES_LIB_ZONE_FILES:$OZONES_LOCATION/Server/lib/OZones
    OZONES_LIB_CLIENT_FILES:$OZONES_LOCATION/Client/lib
    OZONES_LIB_CLIENT_CLI_FILES:$OZONES_LOCATION/Client/lib/CLI
    OZONES_LIB_CLIENT_CLI_HELPER_FILES:$OZONES_LOCATION/Client/lib/CLI/OZonesHelper    
    OZONES_BIN_CLIENT_FILES:$BIN_LOCATION    
)

INSTALL_ETC_FILES=(
    ETC_FILES:$ETC_LOCATION
    VMM_EC2_ETC_FILES:$ETC_LOCATION/vmm_ec2
    VMM_SSH_ETC_FILES:$ETC_LOCATION/vmm_ssh
    VMM_SH_ETC_FILES:$ETC_LOCATION/vmm_sh
    IM_EC2_ETC_FILES:$ETC_LOCATION/im_ec2
    TM_NFS_ETC_FILES:$ETC_LOCATION/tm_nfs
    TM_SSH_ETC_FILES:$ETC_LOCATION/tm_ssh
    TM_DUMMY_ETC_FILES:$ETC_LOCATION/tm_dummy
    TM_LVM_ETC_FILES:$ETC_LOCATION/tm_lvm
    HM_ETC_FILES:$ETC_LOCATION/hm
    AUTH_ETC_FILES:$ETC_LOCATION/auth
    ECO_ETC_FILES:$ETC_LOCATION
    ECO_ETC_TEMPLATE_FILES:$ETC_LOCATION/ec2query_templates
    OCCI_ETC_FILES:$ETC_LOCATION
    OCCI_ETC_TEMPLATE_FILES:$ETC_LOCATION/occi_templates
    SUNSTONE_ETC_FILES:$ETC_LOCATION
    OZONES_ETC_FILES:$ETC_LOCATION
)

#-------------------------------------------------------------------------------
# Binary files, to be installed under $BIN_LOCATION
#-------------------------------------------------------------------------------

BIN_FILES="src/nebula/oned \
           src/scheduler/src/sched/mm_sched \
           src/cli/onevm \
           src/cli/onehost \
           src/cli/onevnet \
           src/cli/oneuser \
           src/cli/oneimage \
           src/cli/onetemplate \
           src/cli/onegroup \
           src/cli/onedb \
           share/scripts/one \
           src/authm_mad/oneauth"

#-------------------------------------------------------------------------------
# C/C++ OpenNebula API Library & Development files
# Include files, to be installed under $INCLUDE_LOCATION
# Library files, to be installed under $LIB_LOCATION
#-------------------------------------------------------------------------------

INCLUDE_FILES=""
LIB_FILES=""

#-------------------------------------------------------------------------------
# Ruby library files, to be installed under $LIB_LOCATION/ruby
#-------------------------------------------------------------------------------

RUBY_LIB_FILES="src/mad/ruby/ActionManager.rb \
                src/mad/ruby/CommandManager.rb \
                src/mad/ruby/OpenNebulaDriver.rb \
                src/mad/ruby/VirtualMachineDriver.rb \
                src/mad/ruby/Ganglia.rb \
                src/cli/client_utilities.rb \
                src/cli/command_parse.rb \
                src/oca/ruby/OpenNebula.rb \
                src/tm_mad/TMScript.rb \
                src/authm_mad/one_usage.rb \
                src/authm_mad/quota.rb \
                src/authm_mad/simple_auth.rb \
                src/authm_mad/simple_permissions.rb \
                src/authm_mad/ssh_auth.rb"

RUBY_OPENNEBULA_LIB_FILES="src/oca/ruby/OpenNebula/Host.rb \
                           src/oca/ruby/OpenNebula/HostPool.rb \
                           src/oca/ruby/OpenNebula/Pool.rb \
                           src/oca/ruby/OpenNebula/User.rb \
                           src/oca/ruby/OpenNebula/UserPool.rb \
                           src/oca/ruby/OpenNebula/VirtualMachine.rb \
                           src/oca/ruby/OpenNebula/VirtualMachinePool.rb \
                           src/oca/ruby/OpenNebula/VirtualNetwork.rb \
                           src/oca/ruby/OpenNebula/VirtualNetworkPool.rb \
                           src/oca/ruby/OpenNebula/Image.rb \
                           src/oca/ruby/OpenNebula/ImagePool.rb \
                           src/oca/ruby/OpenNebula/Template.rb \
                           src/oca/ruby/OpenNebula/TemplatePool.rb \
                           src/oca/ruby/OpenNebula/Group.rb \
                           src/oca/ruby/OpenNebula/GroupPool.rb \
                           src/oca/ruby/OpenNebula/XMLUtils.rb"


#-----------------------------------------------------------------------------
# MAD Script library files, to be installed under $LIB_LOCATION/<script lang>
# and remotes directory
#-----------------------------------------------------------------------------

MAD_SH_LIB_FILES="src/mad/sh/scripts_common.sh"
MAD_RUBY_LIB_FILES="src/mad/ruby/scripts_common.rb"

#-------------------------------------------------------------------------------
# Driver executable files, to be installed under $LIB_LOCATION/mads
#-------------------------------------------------------------------------------

MADS_LIB_FILES="src/mad/sh/madcommon.sh \
              src/tm_mad/tm_common.sh \
              src/vmm_mad/ssh/one_vmm_ssh.rb \
              src/vmm_mad/ssh/one_vmm_ssh \
              src/vmm_mad/sh/one_vmm_sh.rb \
              src/vmm_mad/sh/one_vmm_sh \
              src/vmm_mad/ec2/one_vmm_ec2.rb \
              src/vmm_mad/ec2/one_vmm_ec2 \
              src/vmm_mad/dummy/one_vmm_dummy.rb \
              src/vmm_mad/dummy/one_vmm_dummy \
              src/im_mad/im_ssh/one_im_ssh.rb \
              src/im_mad/im_ssh/one_im_ssh \
              src/im_mad/im_sh/one_im_sh.rb \
              src/im_mad/im_sh/one_im_sh \
              src/im_mad/ec2/one_im_ec2.rb \
              src/im_mad/ec2/one_im_ec2 \
              src/im_mad/dummy/one_im_dummy.rb \
              src/im_mad/dummy/one_im_dummy \
              src/tm_mad/one_tm \
              src/tm_mad/one_tm.rb \
              src/hm_mad/one_hm.rb \
              src/hm_mad/one_hm \
              src/authm_mad/one_auth_mad.rb \
              src/authm_mad/one_auth_mad \
              src/image_mad/one_image.rb \
              src/image_mad/one_image"

#-------------------------------------------------------------------------------
# VMM SH Driver KVM scripts, to be installed under $REMOTES_LOCATION/vmm/kvm
#-------------------------------------------------------------------------------

VMM_SSH_KVM_SCRIPTS="src/vmm_mad/remotes/kvm/cancel \
                    src/vmm_mad/remotes/kvm/deploy \
                    src/vmm_mad/remotes/kvm/kvmrc \
                    src/vmm_mad/remotes/kvm/migrate \
                    src/vmm_mad/remotes/kvm/restore \
                    src/vmm_mad/remotes/kvm/save \
                    src/vmm_mad/remotes/kvm/shutdown"

#-------------------------------------------------------------------------------
# VMM SH Driver Xen scripts, to be installed under $REMOTES_LOCATION/vmm/xen
#-------------------------------------------------------------------------------

VMM_SSH_XEN_SCRIPTS="src/vmm_mad/remotes/xen/cancel \
                    src/vmm_mad/remotes/xen/deploy \
                    src/vmm_mad/remotes/xen/xenrc \
                    src/vmm_mad/remotes/xen/migrate \
                    src/vmm_mad/remotes/xen/restore \
                    src/vmm_mad/remotes/xen/save \
                    src/vmm_mad/remotes/xen/shutdown"

#-----------------------------------------------------------------------------
# VMM SH Driver xen/kvm scripts, to be installed under $REMOTES_LOCATION/vmm/*
#-----------------------------------------------------------------------------

VMM_SSH_XEN_KVM_POLL="src/vmm_mad/remotes/poll_xen_kvm.rb"
VMM_SSH_GANGLIA_POLL="src/vmm_mad/remotes/poll_ganglia.rb"

#-------------------------------------------------------------------------------
# Information Manager Probes, to be installed under $LIB_LOCATION/remotes
#-------------------------------------------------------------------------------

IM_PROBES_FILES="src/im_mad/remotes/run_probes"

IM_PROBES_XEN_FILES="src/im_mad/remotes/xen.d/xen.rb \
                    src/im_mad/remotes/xen.d/architecture.sh \
                    src/im_mad/remotes/xen.d/cpu.sh \
                    src/im_mad/remotes/xen.d/name.sh"

IM_PROBES_KVM_FILES="src/im_mad/remotes/kvm.d/kvm.rb \
                    src/im_mad/remotes/kvm.d/architecture.sh \
                    src/im_mad/remotes/kvm.d/cpu.sh \
                    src/im_mad/remotes/kvm.d/name.sh"

IM_PROBES_GANGLIA_FILES="src/im_mad/remotes/ganglia.d/ganglia_probe"


#-------------------------------------------------------------------------------
# Transfer Manager commands, to be installed under $LIB_LOCATION/tm_commands
#   - NFS TM, $LIB_LOCATION/tm_commands/nfs
#   - SSH TM, $LIB_LOCATION/tm_commands/ssh
#   - dummy TM, $LIB_LOCATION/tm_commands/dummy
#   - LVM TM, $LIB_LOCATION/tm_commands/lvm
#-------------------------------------------------------------------------------

NFS_TM_COMMANDS_LIB_FILES="src/tm_mad/nfs/tm_clone.sh \
                           src/tm_mad/nfs/tm_delete.sh \
                           src/tm_mad/nfs/tm_ln.sh \
                           src/tm_mad/nfs/tm_mkswap.sh \
                           src/tm_mad/nfs/tm_mkimage.sh \
                           src/tm_mad/nfs/tm_mv.sh \
                           src/tm_mad/nfs/tm_context.sh"

SSH_TM_COMMANDS_LIB_FILES="src/tm_mad/ssh/tm_clone.sh \
                           src/tm_mad/ssh/tm_delete.sh \
                           src/tm_mad/ssh/tm_ln.sh \
                           src/tm_mad/ssh/tm_mkswap.sh \
                           src/tm_mad/ssh/tm_mkimage.sh \
                           src/tm_mad/ssh/tm_mv.sh \
                           src/tm_mad/ssh/tm_context.sh"

DUMMY_TM_COMMANDS_LIB_FILES="src/tm_mad/dummy/tm_dummy.sh"

LVM_TM_COMMANDS_LIB_FILES="src/tm_mad/lvm/tm_clone.sh \
                           src/tm_mad/lvm/tm_delete.sh \
                           src/tm_mad/lvm/tm_ln.sh \
                           src/tm_mad/lvm/tm_mkswap.sh \
                           src/tm_mad/lvm/tm_mkimage.sh \
                           src/tm_mad/lvm/tm_mv.sh \
                           src/tm_mad/lvm/tm_context.sh"

#-------------------------------------------------------------------------------
# Image Repository drivers, to be installed under $REMOTES_LOCTION/image
#   - FS based Image Repository, $REMOTES_LOCATION/image/fs
#-------------------------------------------------------------------------------
IMAGE_DRIVER_FS_SCRIPTS="src/image_mad/remotes/fs/cp \
                         src/image_mad/remotes/fs/mkfs \
                         src/image_mad/remotes/fs/mv \
                         src/image_mad/remotes/fs/fsrc \
                         src/image_mad/remotes/fs/rm"

#-------------------------------------------------------------------------------
# Migration scripts for onedb command, to be installed under $LIB_LOCATION
#-------------------------------------------------------------------------------
ONEDB_MIGRATOR_FILES="src/onedb/1.rb"

#-------------------------------------------------------------------------------
# Configuration files for OpenNebula, to be installed under $ETC_LOCATION
#-------------------------------------------------------------------------------

ETC_FILES="share/etc/oned.conf \
           share/etc/defaultrc"

#-------------------------------------------------------------------------------
# Virtualization drivers config. files, to be installed under $ETC_LOCATION
#   - ec2, $ETC_LOCATION/vmm_ec2
#   - sh, $ETC_LOCATION/vmm_sh
#   - ssh, $ETC_LOCATION/vmm_ssh
#-------------------------------------------------------------------------------

VMM_EC2_ETC_FILES="src/vmm_mad/ec2/vmm_ec2rc \
                   src/vmm_mad/ec2/vmm_ec2.conf"

VMM_SSH_ETC_FILES="src/vmm_mad/ssh/vmm_sshrc \
                  src/vmm_mad/ssh/vmm_ssh_kvm.conf \
                  src/vmm_mad/ssh/vmm_ssh_xen.conf"

VMM_SH_ETC_FILES="src/vmm_mad/sh/vmm_shrc"

#-------------------------------------------------------------------------------
# Information drivers config. files, to be installed under $ETC_LOCATION
#   - ec2, $ETC_LOCATION/im_ec2
#-------------------------------------------------------------------------------

IM_EC2_ETC_FILES="src/im_mad/ec2/im_ec2rc \
                  src/im_mad/ec2/im_ec2.conf"

#-------------------------------------------------------------------------------
# Storage drivers config. files, to be installed under $ETC_LOCATION
#   - nfs, $ETC_LOCATION/tm_nfs
#   - ssh, $ETC_LOCATION/tm_ssh
#   - dummy, $ETC_LOCATION/tm_dummy
#   - lvm, $ETC_LOCATION/tm_lvm
#-------------------------------------------------------------------------------

TM_NFS_ETC_FILES="src/tm_mad/nfs/tm_nfs.conf \
                  src/tm_mad/nfs/tm_nfsrc"

TM_SSH_ETC_FILES="src/tm_mad/ssh/tm_ssh.conf \
                  src/tm_mad/ssh/tm_sshrc"

TM_DUMMY_ETC_FILES="src/tm_mad/dummy/tm_dummy.conf \
                    src/tm_mad/dummy/tm_dummyrc"

TM_LVM_ETC_FILES="src/tm_mad/lvm/tm_lvm.conf \
                  src/tm_mad/lvm/tm_lvmrc"

#-------------------------------------------------------------------------------
# Hook Manager driver config. files, to be installed under $ETC_LOCATION/hm
#-------------------------------------------------------------------------------

HM_ETC_FILES="src/hm_mad/hmrc"

#-------------------------------------------------------------------------------
# Hook Manager driver config. files, to be installed under $ETC_LOCATION/hm
#-------------------------------------------------------------------------------

AUTH_ETC_FILES="src/authm_mad/auth_mad \
                src/authm_mad/auth.conf"

#-------------------------------------------------------------------------------
# Sample files, to be installed under $SHARE_LOCATION/examples
#-------------------------------------------------------------------------------

EXAMPLE_SHARE_FILES="share/examples/vm.template \
                     share/examples/private.net \
                     share/examples/public.net"

#-------------------------------------------------------------------------------
# TM Sample files, to be installed under $SHARE_LOCATION/examples/tm
#-------------------------------------------------------------------------------

TM_EXAMPLE_SHARE_FILES="share/examples/tm/tm_clone.sh \
                        share/examples/tm/tm_delete.sh \
                        share/examples/tm/tm_ln.sh \
                        share/examples/tm/tm_mkimage.sh \
                        share/examples/tm/tm_mkswap.sh \
                        share/examples/tm/tm_mv.sh"

#-------------------------------------------------------------------------------
# HOOK scripts, to be installed under $SHARE_LOCATION/hooks
#-------------------------------------------------------------------------------

HOOK_SHARE_FILES="share/hooks/ebtables-xen \
                  share/hooks/ebtables-kvm \
                  share/hooks/ebtables-flush \
                  share/hooks/host_error.rb \
                  share/hooks/image.rb"

INSTALL_NOVNC_SHARE_FILE="share/install_novnc.sh"

#-------------------------------------------------------------------------------
# Common Cloud Files
#-------------------------------------------------------------------------------

COMMON_CLOUD_LIB_FILES="src/cloud/common/CloudServer.rb \
                        src/cloud/common/CloudClient.rb \
                        src/cloud/common/Configuration.rb"

COMMON_CLOUD_CLIENT_LIB_FILES="src/cloud/common/CloudClient.rb"

#-------------------------------------------------------------------------------
# EC2 Query for OpenNebula
#-------------------------------------------------------------------------------

ECO_LIB_FILES="src/cloud/ec2/lib/EC2QueryClient.rb \
               src/cloud/ec2/lib/EC2QueryServer.rb \
               src/cloud/ec2/lib/ImageEC2.rb \
               src/cloud/ec2/lib/econe-server.rb"

ECO_LIB_CLIENT_FILES="src/cloud/ec2/lib/EC2QueryClient.rb"

ECO_LIB_VIEW_FILES="src/cloud/ec2/lib/views/describe_images.erb \
                    src/cloud/ec2/lib/views/describe_instances.erb \
                    src/cloud/ec2/lib/views/register_image.erb \
                    src/cloud/ec2/lib/views/run_instances.erb \
                    src/cloud/ec2/lib/views/terminate_instances.erb"

ECO_BIN_FILES="src/cloud/ec2/bin/econe-server \
               src/cloud/ec2/bin/econe-describe-images \
               src/cloud/ec2/bin/econe-describe-instances \
               src/cloud/ec2/bin/econe-register \
               src/cloud/ec2/bin/econe-run-instances \
               src/cloud/ec2/bin/econe-terminate-instances \
               src/cloud/ec2/bin/econe-upload"

ECO_BIN_CLIENT_FILES="src/cloud/ec2/bin/econe-describe-images \
               src/cloud/ec2/bin/econe-describe-instances \
               src/cloud/ec2/bin/econe-register \
               src/cloud/ec2/bin/econe-run-instances \
               src/cloud/ec2/bin/econe-terminate-instances \
               src/cloud/ec2/bin/econe-upload"

ECO_ETC_FILES="src/cloud/ec2/etc/econe.conf"

ECO_ETC_TEMPLATE_FILES="src/cloud/ec2/etc/templates/m1.small.erb"

#-----------------------------------------------------------------------------
# OCCI files
#-----------------------------------------------------------------------------

OCCI_LIB_FILES="src/cloud/occi/lib/OCCIServer.rb \
                src/cloud/occi/lib/occi-server.rb \
                src/cloud/occi/lib/OCCIClient.rb \
                src/cloud/occi/lib/VirtualMachineOCCI.rb \
                src/cloud/occi/lib/VirtualMachinePoolOCCI.rb \
                src/cloud/occi/lib/VirtualNetworkOCCI.rb \
                src/cloud/occi/lib/VirtualNetworkPoolOCCI.rb \
                src/cloud/occi/lib/ImageOCCI.rb \
                src/cloud/occi/lib/ImagePoolOCCI.rb"

OCCI_LIB_CLIENT_FILES="src/cloud/occi/lib/OCCIClient.rb"

OCCI_BIN_FILES="src/cloud/occi/bin/occi-server \
               src/cloud/occi/bin/occi-compute \
               src/cloud/occi/bin/occi-network \
               src/cloud/occi/bin/occi-storage"

OCCI_BIN_CLIENT_FILES="src/cloud/occi/bin/occi-compute \
               src/cloud/occi/bin/occi-network \
               src/cloud/occi/bin/occi-storage"

OCCI_ETC_FILES="src/cloud/occi/etc/occi-server.conf"

OCCI_ETC_TEMPLATE_FILES="src/cloud/occi/etc/templates/common.erb \
                    src/cloud/occi/etc/templates/custom.erb \
                    src/cloud/occi/etc/templates/small.erb \
                    src/cloud/occi/etc/templates/medium.erb \
                    src/cloud/occi/etc/templates/large.erb"

#-----------------------------------------------------------------------------
# CLI files
#-----------------------------------------------------------------------------

CLI_LIB_FILES="src/mad/ruby/CommandManager.rb \
               src/cli/client_utilities.rb \
               src/cli/command_parse.rb \
               src/oca/ruby/OpenNebula.rb"

CLI_BIN_FILES="src/cli/onevm \
               src/cli/onehost \
               src/cli/onevnet \
               src/cli/oneuser \
               src/cli/oneimage \
               src/cli/onetemplate \
               src/cli/onegroup"

#-----------------------------------------------------------------------------
# Sunstone files
#-----------------------------------------------------------------------------

SUNSTONE_FILES="src/sunstone/config.ru \
                src/sunstone/sunstone-server.rb"

SUNSTONE_BIN_FILES="src/sunstone/bin/sunstone-server"

SUNSTONE_ETC_FILES="src/sunstone/etc/sunstone-server.conf"

SUNSTONE_MODELS_FILES="src/sunstone/models/OpenNebulaJSON.rb \
                       src/sunstone/models/SunstoneServer.rb"

SUNSTONE_MODELS_JSON_FILES="src/sunstone/models/OpenNebulaJSON/HostJSON.rb \
                    src/sunstone/models/OpenNebulaJSON/ImageJSON.rb \
                    src/sunstone/models/OpenNebulaJSON/GroupJSON.rb \
                    src/sunstone/models/OpenNebulaJSON/JSONUtils.rb \
                    src/sunstone/models/OpenNebulaJSON/PoolJSON.rb \
                    src/sunstone/models/OpenNebulaJSON/UserJSON.rb \
                    src/sunstone/models/OpenNebulaJSON/VirtualMachineJSON.rb \
                    src/sunstone/models/OpenNebulaJSON/TemplateJSON.rb \
                    src/sunstone/models/OpenNebulaJSON/VirtualNetworkJSON.rb"

SUNSTONE_TEMPLATE_FILES="src/sunstone/templates/index.html \
                         src/sunstone/templates/login.html"

SUNSTONE_PUBLIC_JS_FILES="src/sunstone/public/js/layout.js \
                        src/sunstone/public/js/login.js \
                        src/sunstone/public/js/sunstone.js \
                        src/sunstone/public/js/sunstone-util.js \
                        src/sunstone/public/js/opennebula.js"

SUNSTONE_PUBLIC_JS_PLUGINS_FILES="\
                            src/sunstone/public/js/plugins/dashboard-tab.js \
                            src/sunstone/public/js/plugins/hosts-tab.js \
                            src/sunstone/public/js/plugins/images-tab.js \
                            src/sunstone/public/js/plugins/templates-tab.js \
                            src/sunstone/public/js/plugins/users-tab.js \
                            src/sunstone/public/js/plugins/vms-tab.js \
                            src/sunstone/public/js/plugins/vnets-tab.js"

SUNSTONE_PUBLIC_CSS_FILES="src/sunstone/public/css/application.css \
                           src/sunstone/public/css/layout.css \
                           src/sunstone/public/css/login.css"

SUNSTONE_PUBLIC_VENDOR_DATATABLES="\
                src/sunstone/public/vendor/dataTables/jquery.dataTables.min.js \
                src/sunstone/public/vendor/dataTables/demo_table_jui.css \
                src/sunstone/public/vendor/dataTables/BSD-LICENSE.txt \
                src/sunstone/public/vendor/dataTables/NOTICE"

SUNSTONE_PUBLIC_VENDOR_JGROWL="\
                src/sunstone/public/vendor/jGrowl/jquery.jgrowl_minimized.js \
                src/sunstone/public/vendor/jGrowl/jquery.jgrowl.css \
                src/sunstone/public/vendor/jGrowl/NOTICE"

SUNSTONE_PUBLIC_VENDOR_JQUERY="\
                        src/sunstone/public/vendor/jQuery/jquery-1.4.4.min.js \
                        src/sunstone/public/vendor/jQuery/MIT-LICENSE.txt \
                        src/sunstone/public/vendor/jQuery/NOTICE"

SUNSTONE_PUBLIC_VENDOR_JQUERYUI="\
src/sunstone/public/vendor/jQueryUI/ui-bg_glass_75_dadada_1x400.png \
src/sunstone/public/vendor/jQueryUI/ui-icons_cd0a0a_256x240.png \
src/sunstone/public/vendor/jQueryUI/jquery-ui-1.8.7.custom.css \
src/sunstone/public/vendor/jQueryUI/ui-bg_flat_0_aaaaaa_40x100.png \
src/sunstone/public/vendor/jQueryUI/ui-bg_flat_0_8f9392_40x100.png \
src/sunstone/public/vendor/jQueryUI/MIT-LICENSE.txt \
src/sunstone/public/vendor/jQueryUI/jquery-ui-1.8.7.custom.min.js \
src/sunstone/public/vendor/jQueryUI/ui-bg_highlight-soft_75_cccccc_1x100.png \
src/sunstone/public/vendor/jQueryUI/ui-bg_glass_95_fef1ec_1x400.png \
src/sunstone/public/vendor/jQueryUI/ui-bg_glass_55_fbf9ee_1x400.png \
src/sunstone/public/vendor/jQueryUI/ui-icons_888888_256x240.png \
src/sunstone/public/vendor/jQueryUI/ui-bg_glass_75_e6e6e6_1x400.png \
src/sunstone/public/vendor/jQueryUI/ui-bg_flat_0_575c5b_40x100.png \
src/sunstone/public/vendor/jQueryUI/ui-bg_glass_65_ffffff_1x400.png \
src/sunstone/public/vendor/jQueryUI/ui-bg_flat_75_ffffff_40x100.png \
src/sunstone/public/vendor/jQueryUI/ui-icons_2e83ff_256x240.png \
src/sunstone/public/vendor/jQueryUI/ui-icons_454545_256x240.png \
src/sunstone/public/vendor/jQueryUI/NOTICE \
src/sunstone/public/vendor/jQueryUI/ui-icons_222222_256x240.png \
"
SUNSTONE_PUBLIC_VENDOR_JQUERYLAYOUT="\
            src/sunstone/public/vendor/jQueryLayout/layout-default-latest.css \
            src/sunstone/public/vendor/jQueryLayout/jquery.layout.min-1.2.0.js \
            src/sunstone/public/vendor/jQueryLayout/NOTICE"

SUNSTONE_PUBLIC_IMAGES_FILES="src/sunstone/public/images/ajax-loader.gif \
                        src/sunstone/public/images/login_over.png \
                        src/sunstone/public/images/login.png \
                        src/sunstone/public/images/opennebula-sunstone-big.png \
                        src/sunstone/public/images/opennebula-sunstone-small.png \
                        src/sunstone/public/images/panel.png \
                        src/sunstone/public/images/pbar.gif \
                        src/sunstone/public/images/Refresh-icon.png \
                        src/sunstone/public/images/vnc_off.png \
                        src/sunstone/public/images/vnc_on.png"

SUNSTONE_RUBY_LIB_FILES="src/mad/ruby/CommandManager.rb \
                         src/oca/ruby/OpenNebula.rb"
                         
                         
#-----------------------------------------------------------------------------
# Ozones files
#-----------------------------------------------------------------------------

OZONES_FILES="src/ozones/Server/config.ru \
              src/ozones/Server/ozones-server.rb"

OZONES_BIN_FILES="src/ozones/Server/bin/ozones-server"

OZONES_ETC_FILES="src/ozones/Server/etc/ozones-server.conf"

OZONES_MODELS_FILES="src/ozones/Server/models/OzonesServer.rb \
                     src/ozones/Server/models/Auth.rb \
                     src/ozones/Server/models/OCAInteraction.rb"
                     
OZONES_LIB_FILES="src/ozones/Server/lib/OZones.rb"

OZONES_LIB_ZONE_FILES="src/ozones/Server/lib/OZones/Zones.rb \
                src/ozones/Server/lib/OZones/VDC.rb \
                src/ozones/Server/lib/OZones/ProxyRules.rb \
                src/ozones/Server/lib/OZones/ApacheWritter.rb \
                src/ozones/Server/lib/OZones/AggregatedHosts.rb \
                src/ozones/Server/lib/OZones/AggregatedUsers.rb \
                src/ozones/Server/lib/OZones/AggregatedVirtualMachines.rb \
                src/ozones/Server/lib/OZones/AggregatedVirtualNetworks.rb \
                src/ozones/Server/lib/OZones/AggregatedPool.rb \
                src/ozones/Server/lib/OZones/AggregatedImages.rb"                                                                                                                
                
OZONES_LIB_CLIENT_FILES="src/ozones/Client/lib/OZonesClient.rb"
                                
OZONES_LIB_CLIENT_CLI_FILES="src/ozones/Client/lib/CLI/OZonesHelper.rb"                   
                
OZONES_LIB_CLIENT_CLI_HELPER_FILES="src/ozones/Client/lib/CLI/OZonesHelper/VDCHelper.rb \
                src/ozones/Client/lib/CLI/OZonesHelper/ZonesHelper.rb"                

OZONES_BIN_CLIENT_FILES="src/ozones/Client/bin/ovdcs \
               src/ozones/Client/bin/ozones"
               
OZONES_RUBY_LIB_FILES="src/oca/ruby/OpenNebula.rb"

#-----------------------------------------------------------------------------
# MAN files
#-----------------------------------------------------------------------------

MAN_FILES="share/man/oneauth.1.gz \
        share/man/onehost.1.gz \
        share/man/oneimage.1.gz \
        share/man/oneuser.1.gz \
        share/man/onevm.1.gz \
        share/man/onevnet.1.gz \
        share/man/onetemplate.1.gz \
        share/man/onedb.1.gz \
        share/man/econe-describe-images.1.gz \
        share/man/econe-describe-instances.1.gz \
        share/man/econe-register.1.gz \
        share/man/econe-run-instances.1.gz \
        share/man/econe-terminate-instances.1.gz \
        share/man/econe-upload.1.gz \
        share/man/occi-compute.1.gz \
        share/man/occi-network.1.gz \
        share/man/occi-storage.1.gz"

#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
# INSTALL.SH SCRIPT
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------

# --- Create OpenNebula directories ---

if [ "$UNINSTALL" = "no" ] ; then
    for d in $MAKE_DIRS; do
        mkdir -p $DESTDIR$d
    done
fi

# --- Install/Uninstall files ---

do_file() {
    if [ "$UNINSTALL" = "yes" ]; then
        rm $2/`basename $1`
    else
        if [ "$LINK" = "yes" ]; then
            ln -s $SRC_DIR/$1 $DESTDIR$2
        else
            cp $SRC_DIR/$1 $DESTDIR$2
        fi
    fi
}


if [ "$CLIENT" = "yes" ]; then
    INSTALL_SET=${INSTALL_CLIENT_FILES[@]}
elif [ "$SUNSTONE" = "yes" ]; then
    INSTALL_SET="${INSTALL_SUNSTONE_RUBY_FILES[@]} ${INSTALL_SUNSTONE_FILES[@]}"
elif [ "$OZONES" = "yes" ]; then
    INSTALL_SET="${INSTALL_OZONES_RUBY_FILES[@]} ${INSTALL_OZONES_FILES[@]}"
else
    INSTALL_SET="${INSTALL_FILES[@]} ${INSTALL_OZONES_FILES[@]} ${INSTALL_SUNSTONE_FILES[@]}"
fi

for i in ${INSTALL_SET[@]}; do
    SRC=$`echo $i | cut -d: -f1`
    DST=`echo $i | cut -d: -f2`

    eval SRC_FILES=$SRC

    for f in $SRC_FILES; do
        do_file $f $DST
    done
done

if [ "$INSTALL_ETC" = "yes" ] ; then
    for i in ${INSTALL_ETC_FILES[@]}; do
        SRC=$`echo $i | cut -d: -f1`
        DST=`echo $i | cut -d: -f2`

        eval SRC_FILES=$SRC

        OLD_LINK=$LINK
        LINK="no"

        for f in $SRC_FILES; do
            do_file $f $DST
        done

        LINK=$OLD_LINK
   done
fi

# --- Set ownership or remove OpenNebula directories ---

if [ "$UNINSTALL" = "no" ] ; then
    for d in $CHOWN_DIRS; do
        chown -R $ONEADMIN_USER:$ONEADMIN_GROUP $DESTDIR$d
    done

    # --- Set correct permissions for Image Repository ---

    if [ -d "$DESTDIR$IMAGES_LOCATION" ]; then
        chmod 3770 $DESTDIR$IMAGES_LOCATION
    fi
else
    for d in `echo $DELETE_DIRS | awk '{for (i=NF;i>=1;i--) printf $i" "}'`; do
        rmdir $d
    done
fi
