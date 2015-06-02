require 'orocos'

include Orocos
Orocos.initialize
Orocos.run 'usbl_evologics::ImProducer' => 'serial_improd',
    'usbl_evologics::Usbl' => 'serial_usbl', 
    'usbl_evologics::BurstDataProducer' => 'serial_burstdataprod' do
    usbl = Orocos.name_service.get 'serial_usbl'
    improd = Orocos.name_service.get 'serial_improd'
    burstdataprod = Orocos.name_service.get 'serial_burstdataprod'
    #USBL Config
    usbl.device_string = "serial:///dev/ttyUSB_USBL:19200" 
    usbl.source_level = 3
    usbl.source_level_control = false
    usbl.low_gain = false
    usbl.carrier_waveform_id = 0
    usbl.local_address = 1
    usbl.remote_address = 2
    usbl.cluster_size = 10
    usbl.packet_time = 750
    usbl.retry_count = 50
    usbl.retry_timeout = 1500
    usbl.idle_timeout = 120
    usbl.sound_speed = 1500
    usbl.im_retry = 100

    #IM Producer Config
    improd.message_content = "Hallo Land - hier spricht das AUV"
    improd.destination = 2
    improd.delivery_report = true

    #Burstdata Producer Config
    burstdataprod.message_content = "Hallo Land - hier spricht das AUV"

    #Connections
    improd.im_output.connect_to usbl.message_input 
    #burstdataprod.burstdata_output.connect_to usbl.burstdata_input

    #Starting
  #  burstdataprod.start
#    improd.start

    usbl.configure
    usbl.start
    #usbl.storePermanently
    while true
#        puts usbl.getConnectionStateAsString
        sleep 0.1
    end
end
