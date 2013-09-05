require 'orocos'

include Orocos
Orocos.initialize
Orocos.run 'usbl_orogen::Usbl' => 'usbl' do
    usbl = Orocos.name_service.get 'usbl'
    usbl.configure
    usbl.start
    while true
#        puts "something"
    end
end
