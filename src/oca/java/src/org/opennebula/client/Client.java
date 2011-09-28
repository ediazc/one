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
package org.opennebula.client;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

import org.apache.xmlrpc.XmlRpcException;
import org.apache.xmlrpc.client.XmlRpcClient;
import org.apache.xmlrpc.client.XmlRpcClientConfigImpl;


/**
 * This class represents the connection with the core and handles the
 * xml-rpc calls.
 *
 */
public class Client{

    //--------------------------------------------------------------------------
    //  PUBLIC INTERFACE
    //--------------------------------------------------------------------------

    /**
     * Creates a new xml-rpc client with default options: the auth. file will be
     * assumed to be at $ONE_AUTH, and the endpoint will be set to $ONE_XMLRPC. <br/>
     * It is the equivalent of Client(null, null).
     *
     * @throws ClientConfigurationException
     *             if the default configuration options are invalid.
     */
    public Client() throws ClientConfigurationException
    {
        setOneAuth(null);
        setOneEndPoint(null);
    }

    /**
     * Creates a new xml-rpc client with specified options.
     *
     * @param secret
     *            A string containing the ONE user:password tuple. Can be null
     * @param endpoint
     *            Where the rpc server is listening, must be something like
     *            "http://localhost:2633/RPC2". Can be null
     * @throws ClientConfigurationException
     *             if the configuration options are invalid
     */
    public Client(String secret, String endpoint)
            throws ClientConfigurationException
    {
        setOneAuth(secret);
        setOneEndPoint(endpoint);
    }

    /**
     * Performs an XML-RPC call.
     *
     * @param action ONE action
     * @param args ONE arguments
     * @return The server's xml-rpc response encapsulated
     */
    public OneResponse call(String action, Object...args)
    {
        boolean success = false;
        String  msg = null;

        try
        {
            Object[] params = new Object[args.length + 1];

            params[0] = oneAuth;

            for(int i=0; i<args.length; i++)
                params[i+1] = args[i];

            Object[] result = (Object[]) client.execute("one."+action, params);

            success = (Boolean) result[0];

            // In some cases, the xml-rpc response only has a boolean
            // OUT parameter
            if(result.length > 1)
            {
                try
                {
                    msg = (String) result[1];
                }
                catch (ClassCastException e)
                {
                    // The result may be an Integer
                    msg = ((Integer) result[1]).toString();
                }
            }


        }
        catch (XmlRpcException e)
        {
            msg = e.getMessage();
        }

        return new OneResponse(success, msg);
    }

    //--------------------------------------------------------------------------
    //  PRIVATE ATTRIBUTES AND METHODS
    //--------------------------------------------------------------------------

    private String       oneAuth;
    private String       oneEndPoint;

    private XmlRpcClient client;

    private void setOneAuth(String secret) throws ClientConfigurationException
    {
        String oneSecret = secret;

        try
        {
            if(oneSecret == null)
            {
                String oneAuthEnv = System.getenv("ONE_AUTH");
                File   authFile;

                if ( oneAuthEnv != null && oneAuthEnv.length() != 0)
                {
                    authFile = new File(oneAuthEnv);
                }
                else
                {
                    authFile = new File(System.getenv("HOME")+"/.one/one_auth");
                }

                oneSecret =
                      (new BufferedReader(new FileReader(authFile))).readLine();
            }

            String[] token = oneSecret.split(":");

            if ( token.length > 2 )
            {
                oneAuth = oneSecret;
            }
            else if ( token.length == 2 )
            {
                MessageDigest md = MessageDigest.getInstance("SHA-1");
                byte[] digest    = md.digest(token[1].getBytes());

                String hash = "";

                for(byte aux : digest)
                {
                    int b = aux & 0xff;

                    if (Integer.toHexString(b).length() == 1)
                    {
                        hash += "0";
                    }

                    hash += Integer.toHexString(b);
                }

                oneAuth = token[0] + ":" + hash;
            }
            else
            {
                throw new ClientConfigurationException(
                    "Wrong format for authorization string: "
                    + oneSecret + "\nFormat expected is user:password");
            }

        }
        catch (FileNotFoundException e)
        {
            // This comes first, since it is a special case of IOException
            throw new ClientConfigurationException("ONE_AUTH file not present");
        }
        catch (IOException e)
        {
            // You could have the file but for some reason the program can not
            // read it
            throw new ClientConfigurationException("ONE_AUTH file unreadable");
        }
        catch (NoSuchAlgorithmException e)
        {
            // A client application cannot recover if the SHA-1 digest
            // algorithm cannot be initialized
            throw new RuntimeException(
                    "Error initializing MessageDigest with SHA-1", e);
        }
    }

    private void setOneEndPoint(String endpoint)
            throws ClientConfigurationException
    {
        oneEndPoint = "http://localhost:2633/RPC2";

        if(endpoint != null)
        {
            oneEndPoint = endpoint;
        }
        else
        {
            String oneXmlRpcEnv = System.getenv("ONE_XMLRPC");

            if ( oneXmlRpcEnv != null && oneXmlRpcEnv.length() != 0 )
            {
                oneEndPoint = oneXmlRpcEnv;
            }
        }

        XmlRpcClientConfigImpl config = new XmlRpcClientConfigImpl();

        try
        {
            config.setServerURL(new URL(oneEndPoint));
        }
        catch (MalformedURLException e)
        {
            throw new ClientConfigurationException(
                    "The URL "+oneEndPoint+" is malformed.");
        }

        client = new XmlRpcClient();
        client.setConfig(config);
    }
}
