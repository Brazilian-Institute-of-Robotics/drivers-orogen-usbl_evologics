require 'orocos'

include Orocos
Orocos.initialize
Orocos.run 'usbl_evologics::UsblDock' => 'usbldock' do
   
    usbl = Orocos.name_service.get 'usbldock'

   #USBL Config
    usbl.io_port = "tcp://192.168.0.191:9200"
#    usbl.reset_drop_counter = false
#    usbl.reset_overflow_counter = false    
        
        
    usbl.configure
    usbl.start

    while true
#        puts "something"
    end
end
