# -------------------------------------------------------------------------- */
# Copyright 2002-2009, Distributed Systems Architecture Group, Universidad   */
# Complutense de Madrid (dsa-research.org)                                   */
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

require 'thread'

=begin rdoc
Copyright 2002-2009, Distributed Systems Architecture Group, Universidad
Complutense de Madrid (dsa-research.org)

This class provides support to handle actions. Class methods, or actions, can be
registered in the action manager. The manager will wait for actions to be
triggered (thread-safe), and will execute them concurrently. The action manager
can be used to synchronize different objects in different threads

== Example

    class Sample
        attr_reader :am

        def initialize
            @am = ActionManager.new(15,true)

            @am.register_action("SLEEP",method("sleep_action"))
        end

        def sleep_action(secs)
            sleep(secs)
        end

        def finalize_action
            p "Exiting..."
            @am.stop_listener
        end
    end


    s = Sample.new

    s.@am.start_listener

#   Objects in other threads can trigger actions like this
#   s.am.trigger_action("SLEEP",rand(3)+1)
#   s.am.trigger_action("FINALIZE")
=end

class ActionManager

    # Creates a new Action Manager
    #
    # +concurrency+ is the maximun number of actions that can run at the same
    # time
    # +threaded+ if true actions will be executed by default in a different
    # thread
    def initialize(concurrency=10, threaded=true)
        @finalize       = false
        @actions        = Hash.new
        @threaded       = threaded

        @concurrency    = concurrency
        @action_queue   = Array.new
        @running_actions= 0

        @threads_mutex  = Mutex.new
        @threads_cond   = ConditionVariable.new
    end

    # Registers a new action in the manager. An action is defined by:
    #
    # +aname+ name of the action, it will identify the action
    # +method+ it's invoked with call. It should be a Proc or Method object
    # +threaded+ execute the action in a new thread
    def register_action(aname, method, threaded=nil)
        threaded ||= @threaded

        @actions[aname]={
            :method     => method,
            :threaded   => threaded
        }
    end

    # Triggers the execution of the action.
    #
    # +aname+ name of the action
    # +aargs+ arguments to call the action
    def trigger_action(aname,*aargs)

        return if @finalize

        @threads_mutex.synchronize {

            if aname == "FINALIZE"
                @finalize = true
                @threads_cond.signal if @running_actions == 0
                return
            end

            if !@actions.has_key?(aname)
                return
            end
            
            method_obj = method(@actions[aname][:method])

            if method_obj.arity != aargs.length
                return
            end

            @action_queue << @actions[aname].merge(:args => aargs)

            if @running_actions < @concurrency
                @threads_cond.signal
            end
        }
    end

    def start_listener
        while true
            @threads_mutex.synchronize {
                while ((@concurrency - @running_actions)==0) ||
                        @action_queue.size==0
                    @threads_cond.wait(@threads_mutex)

                    return if (@finalize && @running_actions == 0)
                end

                run_action
            }
        end
    end

private

    def run_action
        action = @action_queue.shift

        if action
            @running_actions += 1

            if action[:threaded]
                Thread.new {
                    action[:method].call(*action[:args])

                    @threads_mutex.synchronize {
                        @running_actions -= 1
                        @threads_cond.signal
                    }
                }
            else
                action[:method].call(*action[:args])

                @running_actions -= 1
            end
        end
    end
end

if __FILE__ == $0

    class Sample
        attr_reader :am

        def initialize
            @am = ActionManager.new(15,true)

            @am.register_action("SLEEP",method("sleep_action"))
#            @am.register_action("SLEEP",Proc.new{|s,i| p s ; sleep(s)})
            @am.register_action("NOP",method("nop_action"))
       end

        def sleep_action(secs, id)
            p "ID: #{id} sleeping #{secs} seconds"
            sleep(secs)
            p "ID: #{id} Awaken!"
        end

        def nop_action
            p " - Just an action"
        end
    end

    s = Sample.new

    Thread.new {
        sleep 1
        100.times {|n|
            s.am.trigger_action("SLEEP",rand(3)+1,n)
            s.am.trigger_action("NOP")
        }

        s.am.trigger_action("FINALIZE")

        s.am.trigger_action("SLEEP",rand(3)+1,999)
        s.am.trigger_action("SLEEP",rand(3)+1,333)
    }

    s.am.start_listener
end
