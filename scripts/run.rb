#! /usr/bin/env ruby
# -*- coding: utf-8 -*-
# If you want to start the Microsoft Life Cam or the Gumstix camera e-CAM32
# you should use the corresponding ruby run-script. 

require 'orocos'

def display_usbl_raw_io(io_to_device, io_from_device)
    all = Array.new
    while sample = io_to_device.read_new
    	all << ['to-device', sample.time, sample]
    end
    while sample = io_from_device.read_new
    	all << ['from-device', sample.time, sample]
    end
    all.sort_by { |dir, time, sample| time }.
        each { |dir, time, sample| puts "#{dir} #{time} #{sample.to_byte_array[8..-1].inspect}" }
end

include Orocos
Orocos.initialize

# Change this flag to have the script display the raw I/O to and from the USBL device itself
display_raw_io = true

Orocos.run "usbl_evologics::UsblAUV" => "usbl" do

    
    usbl = TaskContext.get 'usbl'

    io_to_device = usbl.io_write_listener.reader type: :buffer, size: 100
    io_from_device = usbl.io_read_listener.reader type: :buffer, size: 100

    usbl.apply_conf_file('usbl_evologics::UsblAUV.yml')

    begin
        usbl.configure
        usbl.start

	    Orocos.watch(usbl) do
	        display_usbl_raw_io(io_to_device, io_from_device) if display_raw_io
	        nil
	    end
    rescue Exception
	display_usbl_raw_io(io_to_device, io_from_device) if display_raw_io
	raise
    end

	
#    require "readline" 
#    Readline.readline()
		      

end
