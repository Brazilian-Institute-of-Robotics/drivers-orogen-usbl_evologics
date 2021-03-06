/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"
#include <rtt/extras/FileDescriptorActivity.hpp>
#include <iodrivers_base/ConfigureGuard.hpp>

using namespace std;
using namespace usbl_evologics;
using iodrivers_base::ConfigureGuard;

Task::Task(std::string const& name)
    : TaskBase(name),
      counter_raw_data_sent(0),
      counter_raw_data_received(0),
      counter_raw_data_dropped(0),
      counter_message_delivered(0),
      counter_message_failed(0),
      counter_message_received(0),
      counter_message_sent(0),
      counter_message_dropped(0),
      counter_message_canceled(0)
{
    _status_period.set(base::Time::fromSeconds(1));
    _timeout_delivery_report.set(base::Time::fromSeconds(10));
    _granularity.set(base::Time::fromMilliseconds(500));
}

Task::Task(std::string const& name, RTT::ExecutionEngine* engine)
    : TaskBase(name, engine),
      counter_raw_data_sent(0),
      counter_raw_data_received(0),
      counter_raw_data_dropped(0),
      counter_message_delivered(0),
      counter_message_failed(0),
      counter_message_received(0),
      counter_message_sent(0),
      counter_message_dropped(0),
      counter_message_canceled(0)
{
    _status_period.set(base::Time::fromSeconds(1));
    _timeout_delivery_report.set(base::Time::fromSeconds(10));
    _granularity.set(base::Time::fromMilliseconds(500));
}

Task::~Task()
{
}

bool Task::setSource_level(::usbl_evologics::SourceLevel const & value)
{

    if(!driver->getSourceLevelControl() && driver->getSourceLevel() != value)
    {
        driver->setSourceLevel(value);
        RTT::log(RTT::Info) << "USBL's source level change to \"" << value << "\"" << RTT::endlog();
    }
    return(usbl_evologics::TaskBase::setSource_level(value));
}

bool Task::setSource_level_control(bool value)
{
    if(driver->getSourceLevelControl() != value)
    {
        driver->setSourceLevelControl(value);
        RTT::log(RTT::Info) << "USBL's source level control change to \"" << (value?"true":"false") << "\"" << RTT::endlog();
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
    RTT::log(RTT::Info) << "Clear transmission buffer. Drop raw data and IM"<< RTT::endlog();
    driver->resetDevice(SEND_BUFFER);
}

// Drop burst data and terminate the acoustic connection
void Task::clearRawDataBuffer(void)
{
    RTT::log(RTT::Info) << "Clear raw data buffer."<< RTT::endlog();
    driver->resetDevice(ACOUSTIC_CONNECTION);
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
    // Guard the configureHook in case of exception
    ConfigureGuard guard(this);

    // Creating the driver object
    driver.reset(new Driver(COMMAND));
    if (!_io_port.get().empty())
        driver->openURI(_io_port.get());
    setDriver(driver.get());
    driver->clear();

    if (! TaskBase::configureHook())
        return false;

    // Set interface
    driver->setInterface(_interface.get());

    RTT::extras::FileDescriptorActivity* fd_activity = getActivity<RTT::extras::FileDescriptorActivity>();
    if(fd_activity)
        fd_activity->setTimeout(_granularity.get().toMilliseconds());

    // Switch device to data mode.
    // If device is already in data mode, it will transmit the command 'ATO' as raw data
    // It makes sure the device is in DATA mode
    driver->switchToDataMode();
    //Set operation mode. Default DATA mode.
    if(driver->getMode() != _mode.get())
        driver->setOperationMode(_mode.get());

    driver->resetDevice(SEND_BUFFER, true);

     // Set System Time for current Time.
    driver->setSystemTimeNow();

    // Initialize source level parameters
    updateDynamicProperties();

    // Get parameters.
    DeviceSettings desired_settings = _desired_device_settings.get();
    current_settings = getDeviceSettings();
    RTT::log(RTT::Info) << "USBL's initial settings"<< endl << getStringOfSettings(current_settings, driver->getSourceLevel(), driver->getSourceLevelControl()) << RTT::endlog();

    // Update parameters.
    if(_change_parameters.get())
    {
        //TODO need validation
        driver->updateDeviceParameters(desired_settings, current_settings);
        current_settings = desired_settings;
        RTT::log(RTT::Info) << "USBL's updated settings"<< endl << getStringOfSettings(current_settings) << RTT::endlog();
    }
    // Hack to get the actual parameters, and in case it's need to update one or other parameters, the user is not required to know how to set the whole configuration.
    else
        _desired_device_settings.set(current_settings);


    // Reset drop & overflow counter if desired.
    resetCounters(_reset_drop_counter.get(), _reset_overflow_counter.get());

    // Log device's information
    VersionNumbers device_info = driver->getFirmwareInformation();
    if(device_info.firmwareVersion.find("1.7") == string::npos)
        RTT::log(RTT::Warning) << "Usbl_evologics Task.cpp. Component was developed for firmware version \"1.7\" and actual version is: \""<< device_info.firmwareVersion <<"\". Be aware of eventual incompatibility." << RTT::endlog();
    RTT::log(RTT::Info) << "USBL's firmware information. Firmware version: "<< device_info.firmwareVersion
            <<"Physical and Data-Link layer protocol: "<< device_info.accousticVersion
            << "Manufacturer: " << device_info.manufacturer <<RTT::endlog();

    // No exception in configureHook
    guard.commit();
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

    // Buffer Message, once usbl doesn't queue messages, and it doesn't send several messages in a row.
    SendIM send_IM;
    while(_message_input.read(send_IM) == RTT::NewData)
        enqueueSendIM(send_IM);
    
    // Enqueue Raw data to be transmitted
    iodrivers_base::RawPacket raw_data_input;
    while(_raw_data_input.read(raw_data_input) == RTT::NewData)
        enqueueSendRawPacket(raw_data_input, current_settings);

    // If there is no status to be updated and no data/IM to be sent, we leave the hook
    if (((base::Time::now() - last_status) <= _status_period.get())
        && queueSendIM.empty() && queueSendRawPacket.empty())
        return;

    // Get connection status and reuse it
    AcousticConnection connection_status = driver->getConnectionStatus();
    updateState(connection_status);
    
    // An internal error has occurred on device. Manual says to reset the device.
    if(connection_status.status == OFFLINE_ALARM)
    {
        RTT::log(RTT::Fatal) << "Usbl_evologics Task.cpp. Device Internal Error. RESET DEVICE" << RTT::endlog();
        exception(DEVICE_INTERNAL_ERROR);
        throw runtime_error("Usbl_evologics Task.cpp. Device Internal Error. RESET DEVICE");
    }

    // Output status
    if((base::Time::now() - last_status) > _status_period.get())
    {
        last_status = base::Time::now();
        _acoustic_connection.write(connection_status);
        _acoustic_channel.write( addStatisticCounters( driver->getAcousticChannelparameters()));
        _message_status.write( addStatisticCounters( checkMessageStatus()));

        // Log Source Level in case the Source Level Control is set (local Source Level establish by remote device).
        if(driver->getSourceLevelControl())
            RTT::log(RTT::Info) << "Current Source Level: \"" << driver->getSourceLevel() << "\"" << RTT::endlog();
    }

    // Transmit enqueued Instant Message
    while( isSendIMAvbl(connection_status))
        sendOneIM();

    // Transmit enqueued Raw Data. Drop if not possible.
    while(!queueSendRawPacket.empty())
    {
        if(isSendRawDataAvbl(connection_status))
            connection_status.freeBuffer[0] += sendOneRawData();
        else
            dropData(connection_status);
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
        RTT::log(RTT::Error) << info <<"\""<< UsblParser::printBuffer(response_info.buffer) << "\"" << RTT::endlog();
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
        RTT::log(RTT::Error) << error_msg << RTT::endlog();
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
    else
        processParticularNotification(notification);
}

void Task::processParticularNotification(NotificationInfo const &notification)
{
    RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. Notification NOT implemented: \"" << UsblParser::printBuffer(notification.buffer) << "\"." << RTT::endlog();
}

MessageStatus Task::processDeliveryReportNotification(NotificationInfo const &notification)
{
    if(notification.notification != DELIVERY_REPORT)
        throw runtime_error("Usbl_evologics Task.cpp. processDeliveryReportNotification did not receive a delivery report.");

    // Check in queue if an Ack is expected.
    if( !waitIMAck(queuePendingIMs))
    {
        exception(OLD_ACK);
        throw runtime_error("Usbl_evologics Task.cpp. Device received a delivered acknowledgment for an old Instant Message");
    }

    // Pop message from queue and confirm the report receipt.
    MessageStatus message_status;
    message_status.sendIm = queuePendingIMs.front();
    queuePendingIMs.pop();

    stringstream error_msg;
    message_status.status = driver->getIMDeliveryReport(notification.buffer);

    if(message_status.status == DELIVERED)
        counter_message_delivered++;
    else if(message_status.status == FAILED)
    {
        counter_message_failed++;
        error_msg << "Usbl_evologics Task.cpp. Device reported a failed delivery for Instant Message: \""
                << UsblParser::printBuffer(message_status.sendIm.buffer) << "\".";
    }
    else if(message_status.status == CANCELED)
    {
        counter_message_canceled++;
        error_msg << "Usbl_evologics Task.cpp. Canceled delivery report for Instant Message: \""
                << UsblParser::printBuffer(message_status.sendIm.buffer) << "\".";
    }
    else
        throw runtime_error("Usbl_evologics Task.cpp. Unknonw delivery report status.");

    if(!error_msg.str().empty())
        RTT::log(RTT::Warning) << error_msg.str() << RTT::endlog();

    message_status.time = base::Time::now();
    return message_status;
}

MessageStatus Task::checkMessageStatus()
{
    MessageStatus message_status;
    message_status.status = driver->getIMDeliveryStatus();
    if(message_status.status == PENDING)
    {
        // Check in queue if an Ack is expected.
        if( queuePendingIMs.empty() )
        {
            exception(OLD_ACK);
            throw runtime_error("Usbl_evologics Task.cpp. Device is waiting an acknowledgment for an old Instant Message");
        }
        message_status.sendIm = queuePendingIMs.front();
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
        if(state() != HUGE_RAW_DATA_INPUT)
            state(HUGE_RAW_DATA_INPUT);
        RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. HUGE_RAW_DATA_INPUT. RawPacket discharged: \""
                             << UsblParser::printBuffer(raw_packet.data)
                             << "\"" << RTT::endlog();
        outputDroppedData(raw_packet, "HUGE_RAW_DATA_INPUT");
        return;
    }
    if(queueSendRawPacket.size() > MAX_QUEUE_RAW_PACKET_SIZE)
    {
        if(state() != FULL_RAW_DATA_QUEUE)
            state(FULL_RAW_DATA_QUEUE);
        RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. FULL_RAW_DATA_QUEUE. RawPacket discharged: \""
                             << UsblParser::printBuffer(raw_packet.data)
                             << "\"" << RTT::endlog();
        outputDroppedData(raw_packet, "FULL_RAW_DATA_QUEUE");
        return;
    }
    if(state() != RUNNING)
        state(RUNNING);
    queueSendRawPacket.push(raw_packet);
}

void Task::enqueueSendIM(SendIM const &sendIM)
{
    // Check size of Message. It can't be bigger than MAX_MSG_SIZE, according device's manual.
    if(sendIM.buffer.size() > MAX_MSG_SIZE)
    {
        if(state() != HUGE_INSTANT_MESSAGE)
            state(HUGE_INSTANT_MESSAGE);
        RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. HUGE_INSTANT_MESSAGE. Message discharged: \""
                             << UsblParser::printBuffer(sendIM.buffer)
                             << "\"" << RTT::endlog();
        outputDroppedIM(sendIM, " HUGE_INSTANT_MESSAGE");
        return;
    }
    // Check queue size.
    if(queueSendIM.size() > MAX_QUEUE_MSG_SIZE)
    {
        if(state() != FULL_IM_QUEUE)
            state(FULL_IM_QUEUE);
        RTT::log(RTT::Error) << "Usbl_evologics Task.cpp. FULL_IM_QUEUE. Message discharged: \""
                             << UsblParser::printBuffer(sendIM.buffer)
                             << "\"" << RTT::endlog();
        outputDroppedIM(sendIM, "FULL_IM_QUEUE");
        return;
    }
    if(state() != RUNNING)
        state(RUNNING);
    queueSendIM.push(sendIM);
}

bool Task::isSendIMAvbl(AcousticConnection const& acoustic_connection)
{
    // TODO define exactly what to do for each acoustic_connection status
    if( acoustic_connection.status != ONLINE && acoustic_connection.status != INITIATION_ESTABLISH && acoustic_connection.status != INITIATION_LISTEN )
        return false;
    // No Instant Message to transmit
    if( queueSendIM.empty())
        return false;
    // Other Instant Message is been transmitted during the expected delivery report timeout
    if( driver->getIMDeliveryStatus() == PENDING &&
        (base::Time::now() - last_im_sent) < _timeout_delivery_report.get())
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

int Task::sendOneRawData(void)
{
    if(queueSendRawPacket.empty())
        return 0;
    if(driver->getMode() == DATA)
    {
        filterRawData(queueSendRawPacket.front().data);
        driver->sendRawData(queueSendRawPacket.front().data);
        int size = queueSendRawPacket.front().data.size();
        counter_raw_data_sent += size;
        queueSendRawPacket.pop();
        return size;
    }
    else
    {
        RTT::log(RTT::Warning) << "Usbl_evologics Task.cpp. Device can not send raw_data in COMMAND mode. Be sure to switch device to DATA mode" << RTT::endlog();
        // Pop packet or let it get full and go to exception??
        // Pop it by now. If it's on COMMAND mode it's known raw packet won't be transmitted and make no reason to delivery an old raw packet if it switches back to DATA.
        queueSendRawPacket.pop();
        return 0;
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
    status.messageCanceled = counter_message_canceled;
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

void Task::outputDroppedIM(SendIM const& dropped_im, std::string const &reason)
{
    counter_message_dropped ++;
    DroppedMessages drop_im;
    drop_im.time = base::Time::now();
    drop_im.droppedIm = dropped_im;
    drop_im.reason = reason;
    drop_im.messageDropped = counter_message_dropped;
    _dropped_message.write(drop_im);
}

void Task::outputDroppedData(iodrivers_base::RawPacket const& dropped_data, std::string const &reason)
{
    counter_raw_data_dropped ++;
    DroppedRawData drop_data;
    drop_data.time = base::Time::now();
    drop_data.droppedData = dropped_data;
    drop_data.reason = reason;
    drop_data.dataDropped = counter_raw_data_dropped;
    _dropped_raw_data.write(drop_data);
}

void Task::dropData(AcousticConnection const& connection)
{
    if(queueSendRawPacket.empty())
        return;
    std::string reason;
    if( connection.status != ONLINE && connection.status != INITIATION_ESTABLISH && connection.status != INITIATION_LISTEN )
        reason = "CONNECTION_STATUS";
    // Check free buffer size
    else if( connection.freeBuffer[0] < queueSendRawPacket.front().data.size())
        reason = "FULL_USBL_BUFFER";
    else
        reason = "UNKNOWN";
    outputDroppedData(queueSendRawPacket.front(), reason);
    queueSendRawPacket.pop();
}

void Task::updateState(AcousticConnection const& connection)
{
    if( connection.status == BACKOFF || connection.status == NOISE || connection.status == DEAF )
    {
        if(state() != UNEXPECTED_ACOUSTIC_CONNECTION)
            state(UNEXPECTED_ACOUSTIC_CONNECTION);
        return;
    }
    // Consider FULL_USBL_BUFFER if less than 5% is free.
    if( connection.freeBuffer[0] < 0.05* current_settings.poolSize[0])
    {
        if(state() != FULL_USBL_BUFFER)
            state(FULL_USBL_BUFFER);
        return;
    }
    if(state() != RUNNING)
        state(RUNNING);
    return;
}
