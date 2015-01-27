require 'orocos'

include Orocos
Orocos.initialize
Orocos.run 'usbl_orogen::ImProducer' => 'serial_improd',
    'usbl_orogen::Usbl' => 'serial_usbl', 
    'usbl_orogen::BurstDataProducer' => 'serial_burstdataprod' do
    usbl = Orocos.name_service.get 'serial_usbl'
    improd = Orocos.name_service.get 'serial_improd'
    burstdataprod = Orocos.name_service.get 'serial_burstdataprod'
    #USBL Config
    usbl.device_string = "serial:///dev/ttyUSB0:19200" 
    usbl.source_level = 3
    usbl.source_level_control = true
    usbl.low_gain = false
    usbl.carrier_waveform_id = 1
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
    improd.destination = 1
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
#        puts usbl.getConnectionStateAsString
        sleep 0.1
    end
end
