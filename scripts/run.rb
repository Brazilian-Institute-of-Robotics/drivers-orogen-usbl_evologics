require 'orocos'

include Orocos
Orocos.initialize
Orocos.run 'usbl_orogen::Usbl' => 'usbl' do
    usbl = Orocos.name_service.get 'usbl'
    usbl.source_level = 0
    #usbl.configure
    usbl.start
    usbl.storePermanently
    while true
#        puts "something"
    end
end
