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

require 'rubygems'
require 'uri'
require 'net/https'
require 'OpenNebula/Configuration'

require 'zona/OZonesJSON'

require 'zona/OZonesPool'
require 'zona/OZonesElement'

require 'zona/ZonePool'
require 'zona/ZoneElement'

require 'zona/VDCPool'
require 'zona/VDCElement'

module Zona

    class Client

        OZONES_VERSION = <<EOT
oZones 1.0
Copyright 2002-2011, OpenNebula Project Leads (OpenNebula.org)

Licensed under the Apache License, Version 2.0 (the "License"); you may
not use this file except in compliance with the License. You may obtain
a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
EOT

        ######################################################################
        # Initialize client library
        ######################################################################
        def initialize(user=nil, pass=nil, endpoint_str=nil,
                       timeout=nil, debug_flag=true)
            @debug   = debug_flag
            @timeout = timeout

            # Server location
            if endpoint_str
                @endpoint =  endpoint_str
            elsif ENV["OZONES_URL"]
                @endpoint = ENV["OZONES_URL"]
            else
                @endpoint = "http://localhost:6121"
            end

            # Autentication
            if user && pass
                @ozonesauth = [user, pass]
            elsif ENV['OZONES_AUTH']
                @ozonesauth=File.read(ENV['OZONES_AUTH']).strip.split(':')
            end

            if !@ozonesauth
                raise "No authorization data present"
            end

            if @ozonesauth.size != 2
                raise "Authorization data malformed"
            end
        end

        #####################################
        # General Resource Request Methods #
        ####################################

        ######################################################################
        # Retieves all elements on a pool
        # :zonetemplate
        ######################################################################
        def get_pool(kind)
            url = URI.parse(@endpoint+"/" + kind)
            req = Net::HTTP::Get.new(url.path)

            req.basic_auth @ozonesauth[0], @ozonesauth[1]

            res = Client.http_start(url, @timeout) {|http|
                http.request(req)
            }

            return Client.parse_error(res, kind)
        end

        ######################################################################
        # Post a new Resource to the relevant OZones Pool
        # :zonetemplate
        ######################################################################
        def post_resource_file(kind, template)
            tmpl_str = File.read(template)
            post_resource_str(kind, tmpl_str)
        end

        def post_resource_str(kind, tmpl_str)
            tmpl_json = Zona.to_body(kind, tmpl_str)
            post_resource(kind, tmpl_json)
        end

        def post_resource(kind, tmpl_json)
            url = URI.parse("#{@endpoint}/#{kind}")

            req = Net::HTTP::Post.new(url.path)
            req.body=tmpl_json

            req.basic_auth @ozonesauth[0], @ozonesauth[1]

            res = Client.http_start(url, @timeout) do |http|
                http.request(req)
            end

            return Client.parse_error(res, kind)
        end

        def put_resource_str(kind, id, tmpl_str)
            tmpl_json = Client.to_body(kind, tmpl_str)
            put_resource(kind, id, tmpl_json)
        end

        def put_resource(kind, id, tmpl_json)
            url = URI.parse("#{@endpoint}/#{kind}/#{id}")

            req = Net::HTTP::Put.new(url.path)
            req.body=tmpl_json

            req.basic_auth @ozonesauth[0], @ozonesauth[1]

            res = Client.http_start(url, @timeout) do |http|
                http.request(req)
            end

            return Client.parse_error(res, kind)
        end

        def get_resource(kind, id)
            url = URI.parse("#{@endpoint}/#{kind}/#{id}")
            req = Net::HTTP::Get.new(url.path)

            req.basic_auth @ozonesauth[0], @ozonesauth[1]

            res = Client.http_start(url, @timeout) {|http|
                http.request(req)
            }

            return Client.parse_error(res, kind)
        end

        def delete_resource(kind, id)
            url = URI.parse("#{@endpoint}/#{kind}/#{id}")
            req = Net::HTTP::Delete.new(url.path)

            req.basic_auth @ozonesauth[0], @ozonesauth[1]

            res = Client.http_start(url, @timeout) {|http|
                http.request(req)
            }

            return Client.parse_error(res, kind)
        end


        # #########################################################################
        # Starts an http connection and calls the block provided. SSL flag
        # is set if needed.
        # #########################################################################
        def self.http_start(url, timeout, &block)
            http = Net::HTTP.new(url.host, url.port)

            if timeout
                http.read_timeout = timeout.to_i
            end

            if url.scheme=='https'
                http.use_ssl = true
                http.verify_mode=OpenSSL::SSL::VERIFY_NONE
            end

            begin
                http.start do |connection|
                    block.call(connection)
                end
            rescue Errno::ECONNREFUSED => e
                str =  "Error connecting to server (#{e.to_s}).\n"
                str << "Server: #{url.host}:#{url.port}"
                return Error.new(str)
            rescue Errno::ETIMEDOUT => e
                str =  "Error timeout connecting to server (#{e.to_s}).\n"
                str << "Server: #{url.host}:#{url.port}"
                return Error.new(str)
            rescue Timeout::Error => e
                str =  "Error timeout while connected to server (#{e.to_s}).\n"
                str << "Server: #{url.host}:#{url.port}"
                return Error.new(str)
            rescue Errno::ENETUNREACH => e
                str = "Error trying to reach network (#{e.to_s}).\n"
                str << "Server: #{url.host}:#{url.port}"
                return Error.new(str)
            end
        end

        def self.parse_error(value, kind)
            if Zona.is_error?(value)
                return value
            else
                if Zona.is_http_error?(value)
                    str = "Operating with #{kind.upcase} failed with HTTP error"
                    str = " " + str + "code: #{value.code}\n"
                    if value.body
                        # Try to extract error message
                        begin
                            str << "Body: " <<
                                OZonesJSON.parse_json(value.body,
                                                            "error")["message"]
                        rescue
                            str.gsub!("\nBody:","")
                        end
                    end
                    return Error.new(str)
                end
            end
            value # If it is not an error, return it as-is
        end

    end

    # ############################################
    # Template helpers
    # ############################################

    def self.to_body(kind, tmpl_str)
        tmpl = OpenNebula::Configuration.new(tmpl_str)
        res  = { "#{kind}" => tmpl.conf }

        return OZonesJSON.to_json(res)
    end

    # #########################################################################
    # Error handling functions
    # #########################################################################

    def self.is_error?(value)
        value.class==Zona::Error
    end

    def self.is_http_error?(value)
        value.class != Net::HTTPOK
    end

    # #########################################################################
    # The Error Class represents a generic error in the Zona
    # library. It contains a readable representation of the error.
    # #########################################################################
    class Error
        attr_reader :message

        # +message+ a description of the error
        def initialize(message=nil)
            @message=message
        end

        def to_s()
            @message
        end
    end

end
