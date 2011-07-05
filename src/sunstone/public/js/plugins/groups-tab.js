/* -------------------------------------------------------------------------- */
/* Copyright 2002-2011, OpenNebula Project Leads (OpenNebula.org)             */
/*                                                                            */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may    */
/* not use this file except in compliance with the License. You may obtain    */
/* a copy of the License at                                                   */
/*                                                                            */
/* http://www.apache.org/licenses/LICENSE-2.0                                 */
/*                                                                            */
/* Unless required by applicable law or agreed to in writing, software        */
/* distributed under the License is distributed on an "AS IS" BASIS,          */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */
/* See the License for the specific language governing permissions and        */
/* limitations under the License.                                             */
/* -------------------------------------------------------------------------- */

var groups_select="";
var group_list_json = {};
var dataTable_groups;


var groups_tab_content =
'<form id="group_form" action="" action="javascript:alert(\'js error!\');">\
  <div class="action_blocks">\
  </div>\
<table id="datatable_groups" class="display">\
  <thead>\
    <tr>\
      <th class="check"><input type="checkbox" class="check_all" value="">All</input></th>\
      <th>ID</th>\
      <th>Name</th>\
      <th>Users</th>\
    </tr>\
  </thead>\
  <tbody id="tbodygroups">\
  </tbody>\
</table>\
</form>';

var create_group_tmpl =
'<form id="create_group_form" action="">\
  <fieldset style="border:none;">\
     <div>\
        <label for="name">Group name:</label>\
        <input type="text" name="name" id="name" /><br />\
      </div>\
  </fieldset>\
  <fieldset>\
      <div class="form_buttons">\
        <button class="button" id="create_group_submit" value="Group.create">Create</button>\
        <button class="button" type="reset" value="reset">Reset</button>\
      </div>\
  </fieldset>\
</form>';


var group_actions = {
    "Group.create" : {
        type: "create",
        call : OpenNebula.Group.create,
        callback : addGroupElement,
        error : onError,
        notify: true
    },

    "Group.create_dialog" : {
        type: "custom",
        call: popUpCreateGroupDialog
    },

    "Group.list" : {
        type: "list",
        call: OpenNebula.Group.list,
        callback: updateGroupsView,
        error: onError,
    },

    // "Group.showinfo" : {
    //     type: "custom",
    //     call: updateGroupInfo
    // },

    "Group.autorefresh" : {
        type: "custom",
        call: function () {
            OpenNebula.Group.list({timeout: true, success: updateGroupsView,error: onError});
        }
    },

    "Group.refresh" : {
        type: "custom",
        call: function() {
            waitingNodes(dataTable_groups);
            Sunstone.runAction("Group.list");
        },
        callback: function(){},
        error: onError,
        notify: false
    },

    "Group.delete" : {
        type: "multiple",
        call : OpenNebula.Group.delete,
        callback : deleteGroupElement,
        error : onError,
        elements: function() { return getSelectedNodes(dataTable_groups); },
        notify:true
    },

    "Group.chown" : {
        type: "multiple",
        call : OpenNebula.Group.chown,
        callback : updateGroupElement,
        elements: function() { return getSelectedNodes(dataTable_groups); },
        error : onError,
        notify:true
    },

}

var group_buttons = {
    "Group.refresh" : {
        type: "image",
        text: "Refresh list",
        img: "images/Refresh-icon.png",
        condition: True
    },
    "Group.create_dialog" : {
        type: "create_dialog",
        text: "+ New Group",
        condition : True
    },
    "Group.chown" : {
        type: "confirm_with_select",
        text: "Change group owner",
        select: function(){return users_select},
        tip: "Select the new group owner:",
        condition : True
    },

    "Group.delete" : {
        type: "action",
        text: "Delete",
        condition : True
    }
};

var groups_tab = {
    title: 'Groups',
    content: groups_tab_content,
    buttons: group_buttons,
    condition: True
}

Sunstone.addActions(group_actions);
Sunstone.addMainTab('groups_tab',groups_tab);

function groupElementArray(group_json){
    var group = group_json.GROUP;

    var users_str="";
    if (group.USERS.ID &&
        group.USERS.ID.constructor == Array){
        for (var i=0; i<group.USERS.ID.length; i++){
            users_str+=getUserName(group.USERS.ID[i])+', ';
        };
        users_str=users_str.slice(0,-2);
    } else if (group.USERS.ID) {
        users_str=getUserName(group.USERS.ID);
    };

    return [
        '<input type="checkbox" id="group_'+group.ID+'" name="selected_items" value="'+group.ID+'"/>',
        group.ID,
        group.NAME,
        users_str ];
}

function groupInfoListener(){
    $('#tbodygroups tr').live("click",function(e){
        //do nothing if we are clicking a checkbox!
        if ($(e.target).is('input')) {return true;}
        var aData = dataTable_groups.fnGetData(this);
        var id = $(aData[0]).val();
        Sunstone.runAction("Group.showinfo",id);
        return false;
    });
}

function updateGroupSelect(){
    groups_select = makeSelectOptions(dataTable_groups,1,2,-1,"",-1);
}

function updateGroupElement(request, group_json){
    var id = group_json.GROUP.ID;
    var element = groupElementArray(group_json);
    updateSingleElement(element,dataTable_groups,'#group_'+id);
    //No need to update select as all items are in it always
}

function deleteGroupElement(request){
    deleteElement(dataTable_groups,'#group_'+req.request.data);
    updateGroupSelect();
}

function addGroupElement(request,group_json){
    var id = group_json.GROUP.ID;
    var element = groupElementArray(group_json);
    addElement(element,dataTable_groups);
    updateGroupSelect();
}

//updates the list
function updateGroupsView(request, group_list){
    group_list_json = group_list;
    var group_list_array = [];

    $.each(group_list,function(){
        group_list_array.push(groupElementArray(this));
    });

    updateView(group_list_array,dataTable_groups);
    updateGroupSelect(group_list);
    updateDashboard("groups",group_list);
}

//Prepares the dialog to create
function setupCreateGroupDialog(){
    $('div#dialogs').append('<div title="Create group" id="create_group_dialog"></div>');
    $('#create_group_dialog').html(create_group_tmpl);
    $('#create_group_dialog').dialog({
        autoOpen: false,
        modal: true,
        width: 400
    });

    $('#create_group_dialog button').button();

    $('#create_group_form').submit(function(){
        var name=$('#name',this).val();
        var group_json = { "group" : { "name" : name }};
        Sunstone.runAction("Group.create",group_json);
        $('#create_group_dialog').dialog('close');
        return false;
    });
}

function popUpCreateGroupDialog(){
    $('#create_group_dialog').dialog('open');
    return false;
}

//Prepares the autorefresh
function setGroupAutorefresh(){
    setInterval(function(){
        var checked = $('input:checked',dataTable_groups.fnGetNodes());
        var  filter = $("#datatable_groups_filter input").attr("value");
        if (!checked.length && !filter.length){
            Sunstone.runAction("Group.autorefresh");
        }
    },INTERVAL+someTime());
}

$(document).ready(function(){
    dataTable_groups = $("#datatable_groups").dataTable({
        "bJQueryUI": true,
        "bSortClasses": false,
        "sPaginationType": "full_numbers",
        "bAutoWidth":false,
        "aoColumnDefs": [
            { "bSortable": false, "aTargets": ["check"] },
            { "sWidth": "60px", "aTargets": [0] },
            { "sWidth": "35px", "aTargets": [1] }
        ]
    });

    dataTable_groups.fnClearTable();
    addElement([
        spinner,
        '','',''],dataTable_groups);

    Sunstone.runAction("Group.list");
    setupCreateGroupDialog();
    setGroupAutorefresh();

    initCheckAllBoxes(dataTable_groups);
    tableCheckboxesListener(dataTable_groups);
})
