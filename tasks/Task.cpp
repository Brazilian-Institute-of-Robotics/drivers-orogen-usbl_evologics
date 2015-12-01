/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"
#include <rtt/extras/FileDescriptorActivity.hpp>

using namespace usbl_evologics;

Task::Task(std::string const& name)
    : TaskBase(name)
{
    _status_period.set(base::Time::fromSeconds(1));
}

Task::Task(std::string const& name, RTT::ExecutionEngine* engine)
    : TaskBase(name, engine)
{
    _status_period.set(base::Time::fromSeconds(1));
}

Task::~Task()
{
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

    // Get parameters.
    DeviceSettings desired_settings = _desired_device_settings.get();
    current_settings = getDeviceSettings();
    RTT::log(RTT::Info) << "USBL's initial settings"<< endl << getStringOfSettings(current_settings) << endl;

    // TODO need check. Arbitrary set the wait time for 5 seconds.
    timeout_delivery_report = base::Time::fromSeconds(5);
    //Set retries counter
    im_retries_counter = current_settings.imRetry;
    old_message_report = true;
    //Initialize Instant Messages counters
    message_status.messageDelivered = 0;
    message_status.messageFailed = 0;
    message_status.messageReceived = 0;
    message_status.messageSent = 0;


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
       _acoustic_channel.write(driver->getAcousticChannelparameters());

       message_status.status = driver->getIMDeliveryStatus();
       if(message_status.status != EMPTY)
           message_status.sendIm = last_send_IM;
       message_status.time = base::Time::now();
       _message_status.write(message_status);

       // Check for the last delivery report.
       if(base::Time::now() - last_delivery_report > timeout_delivery_report && current_settings.imRetry != im_retries_counter)
       {
           RTT::log(RTT::Error) << "Timeout while wait for a delivery report." << std::endl;
           exception(MISSING_DELIVERY_REPORT);
           return;
       }
   }

   SendIM send_IM;
   // Buffer Message, once usbl doesn't queue messages, and it doesn't send several messages in a row.
   while(_message_input.read(send_IM) == RTT::NewData)
   {
       // Check size of Message. It can't be bigger than MAX_MSG_SIZE, according device's manual.
       if(send_IM.buffer.size() > MAX_MSG_SIZE)
           RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Instant Message \""<< UsblParser::printBuffer(send_IM.buffer) << "\" is longer than MAX_MSG_SIZE of \"" << MAX_MSG_SIZE << "\" bytes. It will not be sent to remote device. " << std::endl;
       // Check buffer size.
       else if(queueSendIM.size() > MAX_QUEUE_MSG_SIZE)
           RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Queue of Instant Message has passed it's maximum size of \""<< MAX_QUEUE_MSG_SIZE << "\" and will not be queued. Reduce the rate of sending message." << std::endl;
       else
           queueSendIM.push(send_IM);
   }

   // Get acoustic connection status
   acoustic_connection = driver->getConnectionStatus();

   // TODO define exactly what to do for each acoustic_connection status
   if(acoustic_connection.status == ONLINE || acoustic_connection.status == INITIATION_ESTABLISH || acoustic_connection.status == INITIATION_LISTEN )
   {
       // Only send a new Instant Message if there is no other message been transmitted or if device doesn't wait for report of previously message.
       DeliveryStatus delivery_status;
       while(((delivery_status = driver->getIMDeliveryStatus()) == EMPTY || delivery_status == FAILED) && !queueSendIM.empty() && im_retries_counter == current_settings.imRetry)
       {
           // Check free transmission buffer and instant message size.
           checkFreeBuffer(driver->getStringOfIM(queueSendIM.front()), acoustic_connection);

           // Send latest message in queue.
           driver->sendInstantMessage(queueSendIM.front());
           //Add counter
           message_status.messageSent++;
           // Last send Instant Message
           last_send_IM = queueSendIM.front();
           // If no delivery report is requested, pop message from queue.
           if(!queueSendIM.front().deliveryReport)
               queueSendIM.pop();
       }

       // Transmit raw_data
       iodrivers_base::RawPacket raw_data_input;
       while(_raw_data_input.read(raw_data_input) == RTT::NewData)
       {
           std::string buffer(raw_data_input.data.begin(), raw_data_input.data.end());
           checkFreeBuffer(buffer, acoustic_connection);
           if(driver->getMode() == DATA)
           {
               filterRawData(buffer);
               driver->sendRawData(raw_data_input.data);
           }
           else
               RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Device can not send raw_data \""<< raw_data_input.data.data() << "\" in COMMAND mode. Be sure to switch device to DATA mode" << std::endl;
       }
   }

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
    //TODO set back device to IN_AIR source level to avoid damage device.
    if(current_settings.sourceLevel != IN_AIR)
    {
        current_settings.sourceLevel = IN_AIR;
        driver->setSourceLevel(current_settings.sourceLevel);
    }
    // Clean queueSendIM
    while(!queueSendIM.empty())
        queueSendIM.pop();
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

void Task::checkFreeBuffer(std::string const &buffer, AcousticConnection const &acoustic_connection)
{
    // Only check the first and actual channel.
    if(buffer.size() > acoustic_connection.freeBuffer.at(0))
        // By now, only a message is logged. Let the data be dropped so it will be shown in the output port.
        RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Buffer \"" << UsblParser::printBuffer(buffer) << "\" has size of \"" << buffer.size() << "\" which is bigger than free transmission buffer (\""<< acoustic_connection.freeBuffer.at(0) <<"\"). Split your buffer or reduce the rate of transmission." << std::endl;
}

// TODO verify
// Filter possible <+++ATcommand> in raw_data_input
void Task::filterRawData( std::string const & raw_data_in)
{
    // Find "+++"
    if (raw_data_in.find("+++") != string::npos)
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

    // Pop Message since first delivery report.
    if(!queueSendIM.empty() && im_retries_counter == current_settings.imRetry)
    {
        queueSendIM.pop();
        old_message_report = false;
    }
    last_delivery_report = base::Time::now();

    if(!driver->getIMDeliveryReport(notification.buffer))
    {
        message_status.status = FAILED;
        std::stringstream retries_count;
        retries_count << im_retries_counter;
        std::stringstream error_msg;
        error_msg << "Usbl_evologics Task.cpp. Device did NOT receive a delivered acknowledgment for Instant Message: \""
                << UsblParser::printBuffer(last_send_IM.buffer) << "\". Usbl will make \"" << ((current_settings.imRetry < 255) ? retries_count.str():"Indefinitely")
                << "\" retries to send message";

        // Got a Failed report from a old message
        if(old_message_report)
            error_msg.str(std::string("Usbl_evologics Task.cpp. Device did NOT receive a delivered acknowledgment for an old instant Instant Message"));

        // Decrease counter till it reach 0.
        // In case device does a finite amount of retries
        // In case counter hasn't reached zero
        // In case it is a report from an actual message (not an old one).
        else if(current_settings.imRetry > 0 && current_settings.imRetry < 255
                && im_retries_counter > 0)
            im_retries_counter--;
        // Reset counter in case it reach it's limit
        else if(current_settings.imRetry && im_retries_counter <= 0)
        {
            im_retries_counter = current_settings.imRetry;
            // Add counter when device won't make any more retry and message is definitely FAILED
            message_status.messageFailed++;
        }

        RTT::log(RTT::Error) << error_msg.str() << std::endl;
    }
    else
    {
        message_status.status = DELIVERED;
        message_status.messageDelivered++;
        // Reset counter
        if (im_retries_counter != current_settings.imRetry)
            im_retries_counter = current_settings.imRetry;
    }

    message_status.sendIm = last_send_IM;
    message_status.time = base::Time::now();
    return message_status;
}

std::string Task::getStringOfSettings(DeviceSettings settings)
{
    std::stringstream text;
    text << "Source Level: " << settings.sourceLevel << endl << "Source Level Control: " << (settings.sourceLevelControl?"true":"false") << endl
            << "Low Gain: " << (settings.lowGain?"true":"false") << endl << "Carrier Waveform ID: " << settings.carrierWaveformId << endl
            << "Local Address: " << settings.localAddress << endl << "Remote Address: " << settings.remoteAddress << endl
            << "Highest Address: " << settings.highestAddress << endl << "Cluster Size: " << settings.clusterSize << endl
            << "Packet Time [ms]: " << settings.packetTime << endl << "Retry Count: " << settings.retryCount << endl
            << "Retry Timeout [ms]: " << settings.retryTimeout << endl << "Idle Timeout [s]: " << settings.idleTimeout << endl
            << "Sound Speed [m/s]: " << settings.speedSound << endl << "Instant Message Retry: " << settings.imRetry << endl
            << "Promiscuous Mode: " << (settings.promiscuosMode?"true":"false") << endl << "Wake Up Active Time [s]: " << settings.wuActiveTime << endl
            << "Wake Up Period [s]: " << settings.wuPeriod << endl << "Wake Up Hold Time [s]: " << settings.wuHoldTimeout << endl;
    for(int i=0; i < settings.poolSize.size(); i++)
        text << "Pool size [" << i <<"]: " << settings.poolSize[i];

    return text.str();
}
