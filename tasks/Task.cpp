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
    // Update parameters.
    if(_change_parameters.get())
    {
        //TODO need validation
        driver->updateDeviceParameters(desired_settings, current_settings);
        current_settings = desired_settings;
    }

    // Reset drop & overflow counter if desired.
    resetCounters(_reset_drop_counter.get(), _reset_overflow_counter.get());

    // Log device's information
    VersionNumbers device_info = driver->getFirmwareInformation();
    if(device_info.firmwareVersion.find("1.7") == std::string::npos)
        RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Component was developed for firmware version \"1.7\" and actual version is: \""<< device_info.firmwareVersion <<"\". Be aware of eventual incompatibility." << std::endl;
    RTT::log(RTT::Info) << "USBL's firmware information: Firmware version: \""<< device_info.firmwareVersion <<"\"; Physical and Data-Link layer protocol: \""<<
            device_info.accousticVersion <<"\"; Manufacturer: \""<< device_info.manufacturer << "\"" << std::endl;

    return true;
}
bool Task::startHook()
{
    if (! TaskBase::startHook())
        return false;

    lastStatus = base::Time::now();

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
       MessageStatus message_status;
       message_status.status = driver->getIMDeliveryStatus();
       if(message_status.status != EMPTY && !queueSendIM.empty())
           message_status.sendIm = queueSendIM.front();
       message_status.time = base::Time::now();
       _message_status.write(message_status);
   }

   SendIM send_IM;
   // Buffer Message, once usbl doesn't queue messages, and it doesn't send several messages in a row.
   while(_message_input.read(send_IM) == RTT::NewData)
   {
       // Check size of Message. It can't be bigger than MAX_MSG_SIZE, according device's manual.
       if(send_IM.buffer.size() > MAX_MSG_SIZE)
           RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Instant Message \""<< UsblParser::printBuffer(queueSendIM.front().buffer) << "\" is longer than MAX_MSG_SIZE of \"" << MAX_MSG_SIZE << "\" bytes. It will not be sent to remote device. " << std::endl;
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
       while(((delivery_status = driver->getIMDeliveryStatus()) == EMPTY || delivery_status == FAILED) && !queueSendIM.empty())
       {
           // Check free transmission buffer and instant message size.
           checkFreeBuffer(driver->getStringOfIM(queueSendIM.front()), acoustic_connection);

           // Send latest message in queue.
           driver->sendInstantMessage(queueSendIM.front());
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
       std::cout << "Usbl_evologics Task.cpp. Device Internal Error. DEVICE_INTERNAL_ERROR. RESET DEVICE" << std::endl;
       RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Device Internal Error. RESET DEVICE" << std::endl;
       exception(DEVICE_INTERNAL_ERROR);
       throw;
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

}
void Task::processIO()
{
    // TODO enqueue RawData and Notification
    ResponseInfo response_info;
    if((response_info = driver->readResponse()).response != NO_RESPONSE)
    {
        std::string info = "Usbl_evologics Task.cpp. In processIO, unexpected read a response of a request: ";
        std::cout << info <<"\""<< response_info.buffer << "\"" << std::endl;
        RTT::log(RTT::Error) << info <<"\""<< response_info.buffer << "\"" << std::endl;
    }
    else
        std::cout << "processIO "<< response_info.response << " "<< UsblParser::printBuffer(response_info.buffer) << std::endl;
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
        std::cout << error_msg << std::endl;
        RTT::log(RTT::Error) << error_msg << std::endl;
        exception(MALICIOUS_SEQUENCE_IN_RAW_DATA);
        throw;
    }
}

void Task::processNotification(NotificationInfo const &notification)
{
    if(notification.notification == RECVIM)
    {
        _message_output.write(driver->receiveInstantMessage(notification.buffer));
        return ;
    }
    else if(notification.notification == DELIVERY_REPORT)
    {
        MessageStatus message_status;
        if(!driver->getIMDeliveryReport(notification.buffer))
        {
            message_status.status = FAILED;
            std::stringstream error_msg;
            if(!queueSendIM.empty())
                error_msg << "Usbl_evologics Task.cpp. Device did not receive a delivered acknowledgment for Instant Message: \"" << UsblParser::printBuffer(queueSendIM.front().buffer) << "\". Usbl will make \"" << current_settings.imRetry << "\" retries to send message";
            else
                error_msg << "Usbl_evologics Task.cpp. Device did not receive a delivered acknowledgment for Instant Message. But I don't know from which message is this notification. Maybe from an old one.";
            std::cout << error_msg.str() << std::endl;
            RTT::log(RTT::Error) << error_msg.str() << std::endl;
        }
        else
            message_status.status = DELIVERED;
        if(!queueSendIM.empty())
        {
            message_status.sendIm = queueSendIM.front();
            queueSendIM.pop();
        }
        else
        {
            std::string error_msg = "Usbl_evologics Task.cpp. Device receive a delivered acknowledgment for Instant Message. But I don't know from which message is this notification. Maybe from an old one.";
            std::cout << error_msg << std::endl;
            RTT::log(RTT::Error) << error_msg << std::endl;
        }
        message_status.time = base::Time::now();
        _message_status.write(message_status);
        return ;
    }
    else if(notification.notification == CANCELED_IM)
    {
        std::stringstream error_msg;
        if(!queueSendIM.empty())
            error_msg << "Usbl_evologics Task.cpp. Error sending Instant Message: \"" << UsblParser::printBuffer(send_IM.buffer) << "\". Be sure to wait delivery of last IM.";
        else
            error_msg << "Usbl_evologics Task.cpp. Error sending Instant Message. But I don't know from which message is this notification. Maybe from an old one, or one that doesn't require notification.";
        std::cout << error_msg.str() << std::endl;
        RTT::log(RTT::Error) << error_msg.str() << std::endl;
        return ;
    }
    else
        processParticularNotification(notification);
}

void Task::processParticularNotification(NotificationInfo const &notification)
{
    std::cout << "Usbl_evologics Task.cpp. Notification NOT implemented: \"" << UsblParser::printBuffer(notification.buffer) << "\"." << std::endl;
    RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Notification NOT implemented: \"" << UsblParser::printBuffer(notification.buffer) << "\"." << std::endl;
}
