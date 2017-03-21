require 'orocos'

include Orocos
Orocos.initialize
Orocos.run 'usbl_evologics::UsblDock' => 'usbldock' do
   
    usbl = Orocos.name_service.get 'usbldock'

   #USBL Config
    usbl.apply_conf_file('usbl_evologics::UsblDock.yml')
        
    usbl.configure
    usbl.start

    while true
#        puts "something"
    end
end
