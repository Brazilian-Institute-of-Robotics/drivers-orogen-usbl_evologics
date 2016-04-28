/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"
#include <rtt/extras/FileDescriptorActivity.hpp>

using namespace std;
using namespace usbl_evologics;

Task::Task(std::string const& name)
    : TaskBase(name)
{
    _status_period.set(base::Time::fromSeconds(1));
    _timeout_delivery_report.set(base::Time::fromSeconds(10));

    //Initialize Instant Messages counters
    counter_message_delivered = 0;
    counter_message_failed = 0;
    counter_message_received = 0;
    counter_message_sent = 0;
    // Initialize raw data counters
    counter_raw_data_sent = 0;
    counter_raw_data_received = 0;
}

Task::Task(std::string const& name, RTT::ExecutionEngine* engine)
    : TaskBase(name, engine)
{
    _status_period.set(base::Time::fromSeconds(1));
    _timeout_delivery_report.set(base::Time::fromSeconds(10));

    //Initialize Instant Messages counters
    counter_message_delivered = 0;
    counter_message_failed = 0;
    counter_message_received = 0;
    counter_message_sent = 0;
    // Initialize raw data counters
    counter_raw_data_sent = 0;
    counter_raw_data_received = 0;
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
    std::cout << "openURI" << std::endl;
    setDriver(driver.get());
    std::cout << "setDriver" << std::endl;

    if (! TaskBase::configureHook())
        return false;

    // Set interface
    if(_io_port.get().find("tcp") != string::npos)
        driver->setInterface(SERIAL);
    else if (_io_port.get().find("serial") != string::npos)
        driver->setInterface(SERIAL);
    std::cout << "setInterface" << std::endl;

    driver->resetDevice(SEND_BUFFER, true);
    std::cout << "resetDevice" << std::endl;

     // Set System Time for current Time.
    driver->setSystemTimeNow();
    std::cout << "setTimeNow" << std::endl;

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
    if(device_info.firmwareVersion.find("1.7") == string::npos)
        RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Component was developed for firmware version \"1.7\" and actual version is: \""<< device_info.firmwareVersion <<"\". Be aware of eventual incompatibility." << endl;
    RTT::log(RTT::Info) << "USBL's firmware information. Firmware version: "<< device_info.firmwareVersion
            <<"Physical and Data-Link layer protocol: "<< device_info.accousticVersion
            << "Manufacturer: " << device_info.manufacturer << endl;

    return true;
}
bool Task::startHook()
{
    if (! TaskBase::startHook())
        return false;

    last_status = base::Time::now();
    driver->resetDevice(SEND_BUFFER);

    cout << "USBL working" << endl;

    return true;
}
void Task::updateHook()
{
    TaskBase::updateHook();

    // Output status
   if((base::Time::now() - last_status) > _status_period.get())
   {
       last_status = base::Time::now();
       _acoustic_connection.write(driver->getConnectionStatus());
       _acoustic_channel.write( addStatisticCounters( driver->getAcousticChannelparameters()));
       _message_status.write( addStatisticCounters( checkMessageStatus()));

       // Update Source Level in dynamic property in case the Source Level Control is set (local Source Level establish by remote device).
       if(driver->getSourceLevelControl())
           _source_level.set(driver->getSourceLevel());
   }


   // Buffer Message, once usbl doesn't queue messages, and it doesn't send several messages in a row.
   SendIM send_IM;
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
   while( isSendRawDataAvbl(driver->getConnectionStatus()))
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
       counter_raw_data_received += raw_packet_buffer.data.size();
   }

   // An internal error has occurred on device. Manual says to reset the device.
   if(driver->getConnectionStatus().status == OFFLINE_ALARM)
   {
       RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Device Internal Error. RESET DEVICE" << endl;
       exception(DEVICE_INTERNAL_ERROR);
       return;
   }
}
void Task::errorHook()
{
    TaskBase::errorHook();
}
void Task::stopHook()
{
    driver->resetDevice(SEND_BUFFER);
    TaskBase::stopHook();
}
void Task::cleanupHook()
{
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
    while(!queuePendingIMs.empty())
        queuePendingIMs.pop();
    // Clean queueSendRawPacket
    while(!queueSendRawPacket.empty())
        queueSendRawPacket.pop();

    TaskBase::cleanupHook();
}
void Task::processIO()
{
    // Enqueue RawData and Notification
    ResponseInfo response_info;
    if((response_info = driver->readResponse()).response != NO_RESPONSE)
    {
        string info = "Usbl_evologics Task.cpp. In processIO, unexpected read a response of a request: ";
        RTT::log(RTT::Error) << info <<"\""<< UsblParser::printBuffer(response_info.buffer) << "\"" << endl;
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
void Task::filterRawData( vector<uint8_t> const & raw_data_in)
{
    string buffer(raw_data_in.begin(), raw_data_in.end());
    // Find "+++"
    if (buffer.find("+++") != string::npos)
    {
        string error_msg = "Usbl_evologics Task.cpp. There is the malicious string \"+++\" in raw_data_input. DO NOT use it as it could be interpreted as a command by device.";
        RTT::log(RTT::Error) << error_msg << endl;
        exception(MALICIOUS_SEQUENCE_IN_RAW_DATA);
        throw runtime_error(error_msg);
    }
}

void Task::processNotification(NotificationInfo const &notification)
{
    if(notification.notification == RECVIM)
    {
        _message_output.write(driver->receiveInstantMessage(notification.buffer));
        counter_message_received++;
        return ;
    }
    else if(notification.notification == DELIVERY_REPORT)
    {
        _message_status.write( addStatisticCounters( processDeliveryReportNotification(notification)));
        return ;
    }
    else if(notification.notification == CANCELED_IM)
    {
        stringstream error_msg;
        if(!queuePendingIMs.empty())
            error_msg << "Usbl_evologics Task.cpp. Error sending Instant Message: \"" << UsblParser::printBuffer(queuePendingIMs.front().buffer) << "\". Be sure to wait delivery of last IM.";
        else
            error_msg << "Usbl_evologics Task.cpp. Error sending Instant Message. But I don't know from which message is this notification. Maybe from an old one or one that doesn't require ack notification.";
        RTT::log(RTT::Error) << error_msg.str() << endl;
        return ;
    }
    else
        processParticularNotification(notification);
}

void Task::processParticularNotification(NotificationInfo const &notification)
{
    RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Notification NOT implemented: \"" << UsblParser::printBuffer(notification.buffer) << "\"." << endl;
}

MessageStatus Task::processDeliveryReportNotification(NotificationInfo const &notification)
{
    if(notification.notification != DELIVERY_REPORT)
        throw runtime_error("Usbl_evologics Task.cpp. processDeliveryReportNotification did not receive a delivery report.");

    // Check in queue if a Ack is expected.
    if( !waitIMAck(queuePendingIMs))
    {
        exception(OLD_ACK);
        throw runtime_error("Usbl_evologics Task.cpp. Device received a delivered acknowledgment for an old Instant Message");
    }

    // Pop message from queue and confirm the report receipt.
    MessageStatus message_status;
    message_status.sendIm = queuePendingIMs.front();
    queuePendingIMs.pop();

    if(!driver->getIMDeliveryReport(notification.buffer))
    {
        message_status.status = FAILED;
        counter_message_failed++;

        stringstream error_msg;
        error_msg << "Usbl_evologics Task.cpp. Device reported a failed delivery for Instant Message: \""
                << UsblParser::printBuffer(message_status.sendIm.buffer) << "\".";
        RTT::log(RTT::Error) << error_msg.str() << endl;
    }
    else
    {
        message_status.status = DELIVERED;
        counter_message_delivered++;
    }

    message_status.time = base::Time::now();
    return message_status;
}

MessageStatus Task::checkMessageStatus()
{
    MessageStatus message_status;
    message_status.status = driver->getIMDeliveryStatus();
    if(message_status.status == PENDING)
    {
        // Check in queue if a Ack is expected.
        if( queuePendingIMs.empty() )
        {
            exception(OLD_ACK);
            throw runtime_error("Usbl_evologics Task.cpp. Device is waiting an acknowledgment for an old Instant Message");
        }

        message_status.sendIm = queuePendingIMs.front();
        // Check for the last delivery report.
        if(base::Time::now() - last_im_sent > _timeout_delivery_report.get())
        {
            exception(MISSING_DELIVERY_REPORT);
            throw runtime_error("Timeout while wait for a delivery report.");
        }
    }
    /**
     * Let message_status.sendIm empty case there is no message being transmitted (message_status.status == EMPTY or FAILED).
     */
    message_status.time = base::Time::now();
    return message_status;
}

MessageStatus Task::updateMessageStatusForNonAck(SendIM const& non_ack_required_im)
{
    if(non_ack_required_im.deliveryReport)
        throw runtime_error("Usbl_evologics Task.cpp. updateMessageStatusForNonAck received an ack_required_im.");
    MessageStatus message_status;
    message_status.sendIm = non_ack_required_im;
    message_status.status = DELIVERED;
    message_status.time = base::Time::now();
    return message_status;
}

string Task::getStringOfSettings(DeviceSettings settings)
{
    stringstream text;
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
string Task::getStringOfSettings(DeviceSettings settings, SourceLevel source_level, bool source_level_control)
{
    stringstream text;
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

    SendIM im(queueSendIM.front());
    // Send latest message in queue.
    driver->sendInstantMessage(im);
    queueSendIM.pop();
    //Add counter
    counter_message_sent++;

    // Starting count for the timeout_delivery_report
    if(im.deliveryReport)
    {
	queuePendingIMs.push(im);
        last_im_sent = base::Time::now();
    }
    // If no delivery report is requested, pop message from queue.
    else
    {
        // Register that the non_ack_required_IM was delivered.
        _message_status.write( addStatisticCounters( updateMessageStatusForNonAck(im)));
    }
}

void Task::sendOneRawData(void)
{
    if(queueSendRawPacket.empty())
        return;
    if(driver->getMode() == DATA)
    {
        filterRawData(queueSendRawPacket.front().data);
        driver->sendRawData(queueSendRawPacket.front().data);
        counter_raw_data_sent += queueSendRawPacket.front().data.size();
        queueSendRawPacket.pop();
    }
    else
    {
        RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Device can not send raw_data in COMMAND mode. Be sure to switch device to DATA mode" << endl;
        // Pop packet or let it get full and go to exception??
        // Pop it by now. If it's on COMMAND mode it's known raw packet won't be transmitted and make no reason to delivery a old raw packet if it switch back to DATA.
        queueSendRawPacket.pop();
    }
}

AcousticChannel Task::addStatisticCounters( AcousticChannel const& acoustic_connection)
{
    AcousticChannel channel = acoustic_connection;
    channel.sent_raw_data = counter_raw_data_sent;
    channel.received_raw_data = counter_raw_data_received;
    /** TODO
     * connection.delivered_raw_data is given by device.
     * Manual doesn't says how to reset it.
     * May be the case add counter_raw_data_delivered and compute delivered_raw_data inside the task.
     */
    return channel;
}

MessageStatus Task::addStatisticCounters( MessageStatus const& message_status)
{
    MessageStatus status = message_status;
    status.messageDelivered = counter_message_delivered;
    status.messageFailed = counter_message_failed;
    status.messageReceived = counter_message_received;
    status.messageSent = counter_message_sent;
    return status;
}

bool Task::waitIMAck(queue<SendIM> const& queue_im)
{
    if( queue_im.empty())
        return false;
    else if( !queue_im.front().deliveryReport)
        return false;
    return true;
}
