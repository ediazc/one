require 'OpenNebula/Pool'

module OpenNebula
    class Cluster < PoolElement
        # ---------------------------------------------------------------------
        # Constants and Class Methods
        # ---------------------------------------------------------------------
        CLUSTER_METHODS = {
            :info       => "cluster.info",
            :allocate   => "cluster.allocate",
            :delete     => "cluster.delete",
            :addhost    => "cluster.add",
            :removehost => "cluster.remove",
        }

        # Creates a Cluster description with just its identifier
        # this method should be used to create plain Cluster objects.
        # +id+ the id of the user
        #
        # Example:
        #   cluster = Cluster.new(User.build_xml(3),rpc_client)
        #
        def Cluster.build_xml(pe_id=nil)
            if pe_id
                user_xml = "<CLUSTER><ID>#{pe_id}</ID></CLUSTER>"
            else
                user_xml = "<CLUSTER></CLUSTER>"
            end

            XMLElement.build_xml(user_xml,'CLUSTER')
        end

        # ---------------------------------------------------------------------
        # Class constructor
        # ---------------------------------------------------------------------
        def initialize(xml, client)
            super(xml,client)

            @client = client
        end

        # ---------------------------------------------------------------------
        # XML-RPC Methods for the User Object
        # ---------------------------------------------------------------------
        
        # Retrieves the information of the given Cluster.
        def info()
            super(CLUSTER_METHODS[:info], 'CLUSTER')
        end

        # Allocates a new Cluster in OpenNebula
        #
        # +clustername+ A string containing the name of the Cluster.
        def allocate(clustername)
            super(CLUSTER_METHODS[:allocate], clustername)
        end

        # Deletes the Cluster
        def delete()
            super(CLUSTER_METHODS[:delete])
        end

        # Add a host to the cluster
        #
        # +host_id+ ID of the Host to be added to the Cluster
        def add_host(host_id)
            return Error.new('ID not defined') if !@pe_id

            rc = @client.call(CLUSTER_METHODS[:addhost], host_id.to_i, @pe_id)
            rc = nil if !OpenNebula.is_error?(rc)

            return rc
        end

        # Remove a host from the cluster
        #
        # +host_id+ ID of the Host to be removed from the Cluster
        def remove_host(host_id)
            return Error.new('ID not defined') if !@pe_id

            rc = @client.call(CLUSTER_METHODS[:removehost], host_id.to_i)
            rc = nil if !OpenNebula.is_error?(rc)

            return rc
        end
    end
end
