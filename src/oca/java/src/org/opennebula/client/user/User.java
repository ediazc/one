/*******************************************************************************
 * Copyright 2002-2011, OpenNebula Project Leads (OpenNebula.org)
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/
package org.opennebula.client.user;

import org.opennebula.client.Client;
import org.opennebula.client.OneResponse;
import org.opennebula.client.PoolElement;
import org.w3c.dom.Node;


/**
 * This class represents an OpenNebula User.
 * It also offers static XML-RPC call wrappers.
 */
public class User extends PoolElement{

    private static final String METHOD_PREFIX   = "user.";
    private static final String ALLOCATE        = METHOD_PREFIX + "allocate";
    private static final String INFO            = METHOD_PREFIX + "info";
    private static final String DELETE          = METHOD_PREFIX + "delete";
    private static final String PASSWD          = METHOD_PREFIX + "passwd";
    private static final String CHGRP           = METHOD_PREFIX + "chgrp";
    private static final String CHAUTH          = METHOD_PREFIX + "chauth";
    private static final String UPDATE          = METHOD_PREFIX + "update";
    
    /**
     * Creates a new User representation.
     * 
     * @param id The user id (uid).
     * @param client XML-RPC Client.
     */
    public User(int id, Client client) 
    {
        super(id, client);
    }

    /**
     * @see PoolElement 
     */
    protected User(Node xmlElement, Client client)
    {
        super(xmlElement, client);
    }


    // =================================
    // Static XML-RPC methods
    // =================================

    /**
     * Allocates a new user in OpenNebula.
     * 
     * @param client XML-RPC Client.
     * @param username Username for the new user.
     * @param password Password for the new user 
     * @return If successful the message contains
     * the associated id (int uid) generated for this user.
     */
    public static OneResponse allocate(Client client,
                                       String username,
                                       String password)
    {
        return allocate(client, username, password, "");
    }

    /**
     * Allocates a new user in OpenNebula.
     *
     * @param client XML-RPC Client.
     * @param username Username for the new user.
     * @param password Password for the new user
     * @param auth Auth driver for the new user.
     * @return If successful the message contains
     * the associated id (int uid) generated for this user.
     */
    public static OneResponse allocate(Client client,
                                       String username,
                                       String password,
                                       String auth)
    {
        return client.call(ALLOCATE, username, password, auth);
    }

    /** Retrieves the information of the given user.
     * 
     * @param client XML-RPC Client.
     * @param id The user id (uid) for the user to
     * retrieve the information from.
     * @return if successful the message contains the
     * string with the information about the user returned by OpenNebula.
     */
    public static OneResponse info(Client client, int id)
    {
        return client.call(INFO, id);
    }

    /**
     * Deletes a user from OpenNebula.
     * 
     * @param client XML-RPC Client.
     * @param id The user id (uid) of the target user we want to delete. 
     * @return If an error occurs the error message contains the reason.
     */
    public static OneResponse delete(Client client, int id)
    {
        return client.call(DELETE, id);
    }

    /**
     * Changes the password for the given user.
     * 
     * @param client XML-RPC Client.
     * @param id The user id (uid) of the target user we want to modify.
     * @param password The new password.
     * @return If an error occurs the error message contains the reason.
     */
    public static OneResponse passwd(Client client, int id, String password)
    {
        return client.call(PASSWD, id, password);
    }

    /**
     * Changes the main group of the given user
     * 
     * @param client XML-RPC Client.
     * @param id The user id (uid) of the target user we want to modify.
     * @param gid The new group ID.
     * @return If an error occurs the error message contains the reason.
     */
    public static OneResponse chgrp(Client client, int id, int gid)
    {
        return client.call(CHGRP, id, gid);
    }

    /**
     * Changes the auth driver of the given user
     * 
     * @param client XML-RPC Client.
     * @param id The user id (uid) of the target user we want to modify.
     * @param auth The new auth driver.
     * @return If an error occurs the error message contains the reason.
     */
    public static OneResponse chauth(Client client, int id, String auth)
    {
        return client.call(CHAUTH, id, auth);
    }

    /**
     * Replaces the user template contents.
     *
     * @param client XML-RPC Client.
     * @param id The user id of the target user we want to modify.
     * @param new_template New template contents.
     * @return If successful the message contains the user id.
     */
    public static OneResponse update(Client client, int id, String new_template)
    {
        return client.call(UPDATE, id, new_template);
    }

    // =================================
    // Instanced object XML-RPC methods
    // =================================

    /**
     * Loads the xml representation of the user.
     * The info is also stored internally.
     * 
     * @see User#info(Client, int)
     */
    public OneResponse info()
    {
        OneResponse response = info(client, id);
        super.processInfo(response);

        return response;
    }

    /**
     * Deletes the user from OpenNebula.
     * 
     * @see User#delete(Client, int)
     */
    public OneResponse delete()
    {
        return delete(client, id);
    }

    /**
     * Changes the password for the user.
     * 
     * @param password The new password.
     * @return If an error occurs the error message contains the reason.
     */
    public OneResponse passwd(String password)
    {
        return passwd(client, id, password);
    }

    /**
     * Changes the main group of the given user
     * 
     * @param gid The new group ID.
     * @return If an error occurs the error message contains the reason.
     */
    public OneResponse chgrp(int gid)
    {
        return chgrp(client, id, gid);
    }

    /**
     * Changes the auth driver of the given user
     * 
     * @param auth The new auth driver.
     * @return If an error occurs the error message contains the reason.
     */
    public OneResponse chauth(String auth)
    {
        return chauth(client, id, auth);
    }

    /**
     * Replaces the user template contents.
     *
     * @param new_template New template contents.
     * @return If successful the message contains the user id.
     */
    public OneResponse update(String new_template)
    {
        return update(client, id, new_template);
    }

    // =================================
    // Helpers
    // =================================

    /**
     * Returns true if the user is enabled.
     * 
     * @return True if the user is enabled.
     */
    public boolean isEnabled()
    {
        String enabled = xpath("ENABLED");
        return enabled != null && enabled.equals("1");
    }
}
