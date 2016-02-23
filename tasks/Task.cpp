/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"
#include <rtt/extras/FileDescriptorActivity.hpp>

using namespace usbl_evologics;

Task::Task(std::string const& name)
    : TaskBase(name)
{
    _status_period.set(base::Time::fromSeconds(1));
    _timeout_delivery_report.set(base::Time::fromSeconds(10));

    // Defines the moment of sending new instant message
    im_wait_ack = false;
    //Initialize Instant Messages counters
    message_status.messageDelivered = 0;
    message_status.messageFailed = 0;
    message_status.messageReceived = 0;
    message_status.messageSent = 0;
    // Initialize raw data counters
    sent_raw_data_counter = 0;
    received_raw_data_counter = 0;
}

Task::Task(std::string const& name, RTT::ExecutionEngine* engine)
    : TaskBase(name, engine)
{
    _status_period.set(base::Time::fromSeconds(1));
    _timeout_delivery_report.set(base::Time::fromSeconds(10));

    // Defines the moment of sending new instant message
    im_wait_ack = false;
    //Initialize Instant Messages counters
    message_status.messageDelivered = 0;
    message_status.messageFailed = 0;
    message_status.messageReceived = 0;
    message_status.messageSent = 0;
    // Initialize raw data counters
    sent_raw_data_counter = 0;
    received_raw_data_counter = 0;
}

Task::~Task()
{
}

bool Task::setSource_level(SourceLevel value)
{
    if(driver->getSourceLevel() != value)
    {
        driver->setSourceLevel(value);
        RTT::log(RTT::Info) << "USBL's source level change to \"" << value << "\"" << endl;
    }
 return(usbl_evologics::TaskBase::setSource_level(value));
}

bool Task::setSource_level_control(bool value)
{
    if(driver->getSourceLevelControl() != value)
    {
        driver->setSourceLevelControl(value);
        RTT::log(RTT::Info) << "USBL's source level control change to \"" << (value?"true":"false") << "\"" << endl;
    }
 return(usbl_evologics::TaskBase::setSource_level_control(value));
}

// Reset Device to stored settings and restart it.
void Task::resetDevice(void)
{
    driver->resetDevice(DEVICE);
}

// Clear the transmission buffer - drop raw data and instant message
void Task::clearTransmissionBuffer(void)
{
    driver->resetDevice(SEND_BUFFER);
}

// Store current settings.
void Task::storeSettings(void)
{
    driver->storeCurrentSettings();
}

// Restore Factory Settings and reset device.
void Task::restoreFactorySettings(void)
{
    driver->RestoreFactorySettings();
}


/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.

bool Task::configureHook()
{
    // Creating the driver object
    driver.reset(new Driver());
    if (!_io_port.get().empty())
        driver->openURI(_io_port.get());
    setDriver(driver.get());

    if (! TaskBase::configureHook())
        return false;

    // Set interface
    if(_io_port.get().find("tcp") != std::string::npos)
        driver->setInterface(ETHERNET);
    else if (_io_port.get().find("serial") != std::string::npos)
        driver->setInterface(SERIAL);

     // Set System Time for current Time.
    driver->setSystemTimeNow();

    //Set operation mode. Default DATA mode.
    if(driver->getMode() != _mode.get())
        driver->setOperationMode(_mode.get());

    // Initialize source level parameters
    updateDynamicProperties();

    // Get parameters.
    DeviceSettings desired_settings = _desired_device_settings.get();
    current_settings = getDeviceSettings();
    RTT::log(RTT::Info) << "USBL's initial settings"<< endl << getStringOfSettings(current_settings, driver->getSourceLevel(), driver->getSourceLevelControl()) << endl;

    // Update parameters.
    if(_change_parameters.get())
    {
        //TODO need validation
        driver->updateDeviceParameters(desired_settings, current_settings);
        current_settings = desired_settings;
        RTT::log(RTT::Info) << "USBL's updated settings"<< endl << getStringOfSettings(current_settings) << endl;
    }
    // Hack to get the actual parameters, and in case it's need to update one or other parameters, the user is not required to know how to set the whole configuration.
    else
        _desired_device_settings.set(current_settings);


    // Reset drop & overflow counter if desired.
    resetCounters(_reset_drop_counter.get(), _reset_overflow_counter.get());

    // Log device's information
    VersionNumbers device_info = driver->getFirmwareInformation();
    if(device_info.firmwareVersion.find("1.7") == std::string::npos)
        RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Component was developed for firmware version \"1.7\" and actual version is: \""<< device_info.firmwareVersion <<"\". Be aware of eventual incompatibility." << std::endl;
    RTT::log(RTT::Info) << "USBL's firmware information. Firmware version: "<< device_info.firmwareVersion
            <<"Physical and Data-Link layer protocol: "<< device_info.accousticVersion
            << "Manufacturer: " << device_info.manufacturer << endl;

    return true;
}
bool Task::startHook()
{
    if (! TaskBase::startHook())
        return false;

    lastStatus = base::Time::now();

    cout << "USBL working" << endl;

    return true;
}
void Task::updateHook()
{
    TaskBase::updateHook();

    // Output status
   if((base::Time::now() - lastStatus) > _status_period.get())
   {
       lastStatus = base::Time::now();

       _acoustic_connection.write(driver->getConnectionStatus());

       AcousticChannel acoustic_channel = driver->getAcousticChannelparameters();
       acoustic_channel.sent_raw_data = sent_raw_data_counter;
       acoustic_channel.received_raw_data = received_raw_data_counter;
       _acoustic_channel.write(acoustic_channel);

       message_status.status = driver->getIMDeliveryStatus();
       if(message_status.status != EMPTY)
           message_status.sendIm = last_send_IM;
       else
       {
           SendIM empty_IM;
           message_status.sendIm = empty_IM;
       }
       message_status.time = base::Time::now();
       _message_status.write(message_status);

       // Check for the last delivery report.
       if(base::Time::now() - last_im_sent > _timeout_delivery_report.get() && im_wait_ack)
       {
           RTT::log(RTT::Error) << "Timeout while wait for a delivery report." << std::endl;
           exception(MISSING_DELIVERY_REPORT);
           return;
       }
   }

   SendIM send_IM;
   // Buffer Message, once usbl doesn't queue messages, and it doesn't send several messages in a row.
   while(_message_input.read(send_IM) == RTT::NewData)
       enqueueSendIM(send_IM);

   // Enqueue Raw data to be transmitted
   iodrivers_base::RawPacket raw_data_input;
   while(_raw_data_input.read(raw_data_input) == RTT::NewData)
       enqueueSendRawPacket(raw_data_input, current_settings);

   // Transmit enqueued Instant Message
   while( isSendIMAvbl(driver->getConnectionStatus()))
       sendOneIM();

   // Transmit enqueued Raw Data
   while( isSendRawDataAvbl(acoustic_connection = driver->getConnectionStatus()))
       sendOneRawData();

   // Process received notification from device
   while(driver->hasNotification())
       processNotification(driver->getNotification());

   // Process raw_data from remote device
   while(driver->hasRawData())
   {
       iodrivers_base::RawPacket raw_packet_buffer;
       raw_packet_buffer.time = base::Time::now();
       raw_packet_buffer.data = driver->getRawData();
       _raw_data_output.write(raw_packet_buffer);
       received_raw_data_counter += raw_packet_buffer.data.size();
   }

   // An internal error has occurred on device. Manual says to reset the device.
   if(acoustic_connection.status == OFFLINE_ALARM)
   {
       std::string error_msg = "Usbl_evologics Task.cpp. Device Internal Error. RESET DEVICE";
       RTT::log(RTT::Error) << error_msg << std::endl;
       exception(DEVICE_INTERNAL_ERROR);
       throw std::runtime_error(error_msg);
   }

}
void Task::errorHook()
{
    TaskBase::errorHook();
}
void Task::stopHook()
{
    TaskBase::stopHook();
}
void Task::cleanupHook()
{
    TaskBase::cleanupHook();

    //Make sure to set the device to its default operational mode, DATA.
    if(driver->getMode() == COMMAND)
        driver->switchToDataMode();

    //TODO set back device to MINIMAL/IN_AIR source level to avoid damage device.
    if(driver->getSourceLevelControl())
        driver->setSourceLevelControl(false);
    if(driver->getSourceLevel() != MINIMAL)
        driver->setSourceLevel(MINIMAL);

    // Clean queueSendIM
    while(!queueSendIM.empty())
        queueSendIM.pop();
    // Clean queueSendRawPacket
    while(!queueSendRawPacket.empty())
        queueSendRawPacket.pop();
}
void Task::processIO()
{
    // Enqueue RawData and Notification
    ResponseInfo response_info;
    if((response_info = driver->readResponse()).response != NO_RESPONSE)
    {
        std::string info = "Usbl_evologics Task.cpp. In processIO, unexpected read a response of a request: ";
        RTT::log(RTT::Error) << info <<"\""<< UsblParser::printBuffer(response_info.buffer) << "\"" << std::endl;
    }
}

void Task::resetCounters(bool drop_counter, bool overflow_counter)
{
    // Only takes in account the first and actual channel
    if(drop_counter)
        driver->resetDropCounter();
    // Only takes in account the first and actual channel
    if(overflow_counter)
        driver->resetOverflowCounter();
}

DeviceSettings Task::getDeviceSettings(void)
{
    DeviceSettings current_settings = driver->getCurrentSetting();
    // The settings below are not provided by the method getCurrentSetting().
    current_settings.remoteAddress = driver->getRemoteAddress();
    return current_settings;
}

// TODO verify
// Filter possible <+++ATcommand> in raw_data_input
void Task::filterRawData( std::vector<uint8_t> const & raw_data_in)
{
    std::string buffer(raw_data_in.begin(), raw_data_in.end());
    // Find "+++"
    if (buffer.find("+++") != string::npos)
    {
        std::string error_msg = "Usbl_evologics Task.cpp. There is the malicious string \"+++\" in raw_data_input. DO NOT use it as it could be interpreted as a command by device.";
        RTT::log(RTT::Error) << error_msg << std::endl;
        exception(MALICIOUS_SEQUENCE_IN_RAW_DATA);
        throw std::runtime_error(error_msg);
    }
}

void Task::processNotification(NotificationInfo const &notification)
{
    if(notification.notification == RECVIM)
    {
        _message_output.write(driver->receiveInstantMessage(notification.buffer));
        message_status.messageReceived++;
        return ;
    }
    else if(notification.notification == DELIVERY_REPORT)
    {
        _message_status.write(processDeliveryReportNotification(notification));
        return ;
    }
    else if(notification.notification == CANCELED_IM)
    {
        std::stringstream error_msg;
        if(!queueSendIM.empty())
            error_msg << "Usbl_evologics Task.cpp. Error sending Instant Message: \"" << UsblParser::printBuffer(queueSendIM.front().buffer) << "\". Be sure to wait delivery of last IM.";
        else
            error_msg << "Usbl_evologics Task.cpp. Error sending Instant Message. But I don't know from which message is this notification. Maybe from an old one, like: \""<< UsblParser::printBuffer(last_send_IM.buffer) <<"\"" <<", or one that doesn't require notification.";
        RTT::log(RTT::Error) << error_msg.str() << std::endl;
        return ;
    }
    else
        processParticularNotification(notification);
}

void Task::processParticularNotification(NotificationInfo const &notification)
{
    RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Notification NOT implemented: \"" << UsblParser::printBuffer(notification.buffer) << "\"." << std::endl;
}

MessageStatus Task::processDeliveryReportNotification(NotificationInfo const &notification)
{
    if(notification.notification != DELIVERY_REPORT)
        throw runtime_error("Usbl_evologics Task.cpp. processDeliveryReportNotification did not receive a delivery report.");

    // Got a report from a old message, while it was not expected.
    if(!im_wait_ack)
        RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Device received a delivered acknowledgment for an old Instant Message" << std::endl;

    // Pop message from queue and confirm the report receipt.
    if(!queueSendIM.empty() && im_wait_ack)
    {
        queueSendIM.pop();
        im_wait_ack = false;
    }

    if(!driver->getIMDeliveryReport(notification.buffer))
    {
        message_status.status = FAILED;
        message_status.messageFailed++;

        std::stringstream error_msg;
        error_msg << "Usbl_evologics Task.cpp. Device did NOT receive a delivered acknowledgment for Instant Message: \""
                << UsblParser::printBuffer(last_send_IM.buffer) << "\".";
        RTT::log(RTT::Error) << error_msg.str() << std::endl;
    }
    else
    {
        message_status.status = DELIVERED;
        message_status.messageDelivered++;
    }

    message_status.sendIm = last_send_IM;
    message_status.time = base::Time::now();
    return message_status;
}

std::string Task::getStringOfSettings(DeviceSettings settings)
{
    std::stringstream text;
    text << "Low Gain: " << (settings.lowGain?"true":"false") << endl << "Carrier Waveform ID: " << settings.carrierWaveformId << endl
            << "Local Address: " << settings.localAddress << endl << "Remote Address: " << settings.remoteAddress << endl
            << "Highest Address: " << settings.highestAddress << endl << "Cluster Size: " << settings.clusterSize << endl
            << "Packet Time [ms]: " << settings.packetTime << endl << "Retry Count: " << settings.retryCount << endl
            << "Retry Timeout [ms]: " << settings.retryTimeout << endl << "Idle Timeout [s]: " << settings.idleTimeout << endl
            << "Sound Speed [m/s]: " << settings.speedSound << endl << "Instant Message Retry: " << settings.imRetry << endl
            << "Promiscuous Mode: " << (settings.promiscuosMode?"true":"false") << endl << "Wake Up Active Time [s]: " << settings.wuActiveTime << endl
            << "Wake Up Period [s]: " << settings.wuPeriod << endl << "Wake Up Hold Time [s]: " << settings.wuHoldTimeout << endl;
    for(size_t i=0; i < settings.poolSize.size(); i++)
        text << "Pool size [" << i <<"]: " << settings.poolSize[i];

    return text.str();
}

// Get settings in string for log purpose
std::string Task::getStringOfSettings(DeviceSettings settings, SourceLevel source_level, bool source_level_control)
{
    std::stringstream text;
    text << "Source Level: " << source_level << endl << "Source Level Control: " << (source_level_control?"true":"false") << endl
            << getStringOfSettings(settings);
    return text.str();
}

void Task::enqueueSendRawPacket(iodrivers_base::RawPacket const &raw_packet, DeviceSettings const &pool_size)
{
    // Considering just the first and actual channel. Let a minimal free buffer for delivery an Instant Message.
    if(raw_packet.data.size() > (pool_size.poolSize[0] - MAX_MSG_SIZE))
    {
        exception(HUGE_RAW_DATA_INPUT);
        return;
    }
    if(queueSendRawPacket.size() > MAX_QUEUE_RAW_PACKET_SIZE)
    {
        exception(FULL_RAW_DATA_QUEUE);
        return;
    }
    queueSendRawPacket.push(raw_packet);
}

void Task::enqueueSendIM(SendIM const &sendIM)
{
    // Check size of Message. It can't be bigger than MAX_MSG_SIZE, according device's manual.
     if(sendIM.buffer.size() > MAX_MSG_SIZE)
     {
         exception(HUGE_INSTANT_MESSAGE);
         return;
     }
     // Check queue size.
     if(queueSendIM.size() > MAX_QUEUE_MSG_SIZE)
     {
         exception(FULL_IM_QUEUE);
         return;
     }
     queueSendIM.push(sendIM);
}

bool Task::isSendIMAvbl(AcousticConnection const& acoustic_connection)
{
    // TODO define exactly what to do for each acoustic_connection status
    if( acoustic_connection.status != ONLINE && acoustic_connection.status != INITIATION_ESTABLISH && acoustic_connection.status != INITIATION_LISTEN )
        return false;
    // Other Instant Message is been transmitted
    if( driver->getIMDeliveryStatus() == PENDING)
        return false;
    // No Instant Message to transmit
    if( queueSendIM.empty())
        return false;
    // Wait for ack of previous Instant Message
    if( im_wait_ack)
        return false;
    // Check free buffer size
    if( acoustic_connection.freeBuffer[0] < MAX_MSG_SIZE)
        return false;
    return true;
}

bool Task::isSendRawDataAvbl( AcousticConnection const& acoustic_connection)
{
    // TODO define exactly what to do for each acoustic_connection status
    if( acoustic_connection.status != ONLINE && acoustic_connection.status != INITIATION_ESTABLISH && acoustic_connection.status != INITIATION_LISTEN )
        return false;
    // No Raw Data to transmit
    if( queueSendRawPacket.empty())
        return false;
    // Check free buffer size
    if( acoustic_connection.freeBuffer[0] < queueSendRawPacket.front().data.size())
        return false;
    return true;
}

void Task::sendOneIM(void)
{
    //Check if queue is not empty
    if(queueSendIM.empty())
        return;
    // Send latest message in queue.
    driver->sendInstantMessage(queueSendIM.front());
    //Add counter
    message_status.messageSent++;
    // Last send Instant Message
    last_send_IM = queueSendIM.front();

    // Disable sending message until get a delivery report. Wait for acknowledgment.
    if(queueSendIM.front().deliveryReport)
    {
        im_wait_ack = true;
        last_im_sent = base::Time::now();
    }
    // If no delivery report is requested, pop message from queue.
    else
        queueSendIM.pop();
}

void Task::sendOneRawData(void)
{
    if(queueSendRawPacket.empty())
        return;
    if(driver->getMode() == DATA)
    {
        filterRawData(queueSendRawPacket.front().data);
        driver->sendRawData(queueSendRawPacket.front().data);
        sent_raw_data_counter += queueSendRawPacket.front().data.size();
        queueSendRawPacket.pop();
    }
    else
    {
        RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Device can not send raw_data in COMMAND mode. Be sure to switch device to DATA mode" << std::endl;
        // Pop packet or let it get full and go to exception??
        // Pop it by now. If it's on COMMAND mode it's known raw packet won't be transmitted and make no reason to delivery a old raw packet if it switch back to DATA.
        queueSendRawPacket.pop();
    }
}
