require 'orocos'

include Orocos
Orocos.initialize
Orocos.run 'usbl_evologics::ImProducer' => 'improd',
    'usbl_evologics::Usbl' => 'usbl', 
    'usbl_evologics::BurstDataProducer' => 'burstdataprod' do
    usbl = Orocos.name_service.get 'usbl'
    improd = Orocos.name_service.get 'improd'
    burstdataprod = Orocos.name_service.get 'burstdataprod'
    #USBL Config
    usbl.device_string = "tcp://192.168.0.253:9200" 
    usbl.source_level = 3
    usbl.source_level_control = false
    usbl.low_gain = false
    usbl.carrier_waveform_id = 0
    usbl.local_address = 2
    usbl.remote_address = 1
    usbl.cluster_size = 10
    usbl.packet_time = 750
    usbl.retry_count = 50
    usbl.retry_timeout = 1500
    usbl.idle_timeout = 120
    usbl.sound_speed = 1500
    usbl.im_retry = 100

    #IM Producer Config
    improd.message_content = "Hallo Welt"
    improd.destination = 2
    improd.delivery_report = true

    #Burstdata Producer Config
    burstdataprod.message_content = "Hallo Welt"

    #Connections
    improd.im_output.connect_to usbl.message_input 
    #burstdataprod.burstdata_output.connect_to usbl.burstdata_input

    #Starting
    burstdataprod.start
    improd.start

    usbl.configure
    usbl.start
    #usbl.storePermanently
    while true
#        puts "something"
    end
end
