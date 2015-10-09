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

    // Instant Message manager.
    IM_notification_ack = true;

    // Set System Time for current Time
    driver->setSystemTimeNow();

    //Set operation mode.
    if(driver->getMode() != _mode.get())
        driver->setOperationMode(_mode.get());

    DeviceSettings desired_settings = _desired_device_settings.get();
    DeviceSettings current_settings = getDeviceSettings();

    if(_change_parameters.get())
    {
        //TODO need validation
        updateDeviceParameters(desired_settings, current_settings);
    }


    // Reset drop & overflow counter if desired.
    resetCounters(_reset_drop_counter.get(), _reset_overflow_counter.get());

    VersionNumbers device_info = driver->getFirmwareInformation();
    if(device_info.firmwareVersion.find("1.7") == std::string::npos)
        RTT::log(RTT::Error) << "Component was developed for firmware version \"1.7\" and actual version is: \""<< device_info.firmwareVersion <<"\". Be aware of eventual incompatibility." << std::endl;
    RTT::log(RTT::Info) << "USBL's firmware information: Firmware version: \""<< device_info.firmwareVersion <<"\"; Physical and Data-Link layer protocol: \""<<
            device_info.accousticVersion <<"\"; Manufacturer: \""<< device_info.manufacturer << "\"" << std::endl;

    return true;
}
bool Task::startHook()
{
    if (! TaskBase::startHook())
        return false;

    mLastStatus = base::Time::now();
    return true;
}
void Task::updateHook()
{
    TaskBase::updateHook();

   if((base::Time::now() - mLastStatus) > _status_period.get())
   {
       mLastStatus = base::Time::now();

       _acoustic_connection.write(driver->getConnectionStatus());
       _acoustic_channel.write(getAcousticChannelparameters());
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

    acoustic_connection = driver->getConnectionStatus();

    // TODO define exactly what to do for each acoustic_connection status
    if(acoustic_connection.status == ONLINE || acoustic_connection.status == INITIATION_ESTABLISH || acoustic_connection.status == INITIATION_LISTEN )
    {
        // Need a flow management of data be sent to device.
        // Only send a new Instant Message if a acknowledgment notification of previously message was received if it was required.
        while(IM_notification_ack && _message_input.read(send_IM) == RTT::NewData)
        {
            // Check free transmission buffer and instant message size.
            checkFreeBuffer(driver->getStringOfIM(send_IM), acoustic_connection);
            if(send_IM.buffer.size() > MAX_MSG_SIZE)
                RTT::log(RTT::Error) << "Instant Message \""<< send_IM.buffer << "\" is longer than MAX_MSG_SIZE of \"" << MAX_MSG_SIZE << "\" bytes. It maybe not be sent to remote device " << std::endl;

            driver->sendInstantMessage(send_IM);
            if(send_IM.deliveryReport)
                IM_notification_ack = false;
        }

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
                RTT::log(RTT::Error) << "Device can not send raw_data \""<< raw_data_input.data.data() << "\" in COMMAND mode. Be sure to switch device to DATA mode" << std::endl;
        }
    }

    while(driver->hasNotification())
        processNotification(driver->getNotification());

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
        std::cout << "Device Internal Error. DEVICE_INTERNAL_ERRORRESET DEVICE" << std::endl;
        RTT::log(RTT::Error) << "Device Internal Error. RESET DEVICE" << std::endl;
        exception(DEVICE_INTERNAL_ERROR);
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

void Task::updateDeviceParameters(DeviceSettings const &desired_setting, DeviceSettings const &actual_setting)
{
    if(desired_setting.carrierWaveformId != actual_setting.carrierWaveformId)
        driver->setCarrierWaveformID(desired_setting.carrierWaveformId);

    if(desired_setting.clusterSize != actual_setting.clusterSize)
        driver->setClusterSize(desired_setting.clusterSize);

    if(desired_setting.highestAddress != actual_setting.highestAddress)
        driver->setHighestAddress(desired_setting.highestAddress);

    if(desired_setting.idleTimeout != actual_setting.idleTimeout)
        driver->setIdleTimeout(desired_setting.idleTimeout);

    if(desired_setting.imRetry != actual_setting.imRetry)
        driver->setIMRetry(desired_setting.imRetry);

    if(desired_setting.localAddress != actual_setting.localAddress)
        driver->setLocalAddress(desired_setting.localAddress);

    if(desired_setting.lowGain != actual_setting.lowGain)
        driver->setLowGain(desired_setting.lowGain);

    if(desired_setting.packetTime != actual_setting.packetTime)
        driver->setPacketTime(desired_setting.packetTime);

    if(desired_setting.promiscuosMode != actual_setting.promiscuosMode)
        driver->setPromiscuosMode(desired_setting.promiscuosMode);

    if(desired_setting.remoteAddress != actual_setting.remoteAddress)
        driver->setRemoteAddress(desired_setting.remoteAddress);

    if(desired_setting.retryCount != actual_setting.retryCount)
        driver->setRetryCount(desired_setting.retryCount);

    if(desired_setting.retryTimeout != actual_setting.retryTimeout)
        driver->setRetryTimeout(desired_setting.retryTimeout);

    if(desired_setting.sourceLevel != actual_setting.sourceLevel)
        driver->setSourceLevel(desired_setting.sourceLevel);

    if(desired_setting.sourceLevelControl != actual_setting.sourceLevelControl)
        driver->setSourceLevelcontrol(desired_setting.sourceLevelControl);

    if(desired_setting.speedSound != actual_setting.speedSound)
        driver->setSpeedSound(desired_setting.speedSound);

    if(desired_setting.wuActiveTime != actual_setting.wuActiveTime)
        driver->setWakeUpActiveTime(desired_setting.wuActiveTime);

    if(desired_setting.wuHoldTimeout != actual_setting.wuHoldTimeout)
        driver->setWakeUpHoldTimeout(desired_setting.wuHoldTimeout);

    if(desired_setting.wuPeriod != actual_setting.wuPeriod)
        driver->setWakeUpPeriod(desired_setting.wuPeriod);

    if(!actual_setting.poolSize.empty() && !desired_setting.poolSize.empty())
    {
        // Only takes in account the first and actual channel
        if(desired_setting.poolSize.at(0) != actual_setting.poolSize.at(0))
            driver->setPoolSize(desired_setting.poolSize.at(0));
    }
}

AcousticChannel Task::getAcousticChannelparameters(void)
{
    AcousticChannel channel;
    channel.time = base::Time::now();
    channel.rssi = driver->getRSSI();
    channel.localBitrate = driver->getLocalToRemoteBitrate();
    channel.remoteBitrate = driver->getRemoteToLocalBitrate();
    channel.propagationTime = driver->getPropagationTime();
    channel.relativeVelocity = driver->getRelativeVelocity();
    channel.signalIntegrity = driver->getSignalIntegrity();
    channel.multiPath = driver->getMultipath();
    channel.channelNumber = driver->getChannelNumber();
    channel.dropCount = driver->getDropCounter();
    channel.overflowCounter = driver->getOverflowCounter();

    return channel;
}

void Task::checkFreeBuffer(std::string const &buffer, AcousticConnection const &acoustic_connection)
{
    // Only check the first and actual channel.
    if(buffer.size() > acoustic_connection.freeBuffer.at(0))
        // By now, only a message is logged. Let the data be dropped so it will be shown in the output port.
        RTT::log(RTT::Error) << "Buffer \"" << buffer << "\" is bigger than free transmission buffer. Split your buffer or reduce the rate of transmission." << std::endl;
}

// TODO verify
// Filter possible <+++ATcommand> in raw_data_input
void Task::filterRawData( std::string const & raw_data_in)
{
    // Find "+++"
    if (raw_data_in.find("+++") != string::npos)
    {
        std::cout << "There is the malicious string \"+++\" in raw_data_input. DO NOT use it as it could be interpreted as a command by device." << std::endl;
        RTT::log(RTT::Error) << "There is the malicious string \"+++\" in raw_data_input. DO NOT use it as it could be interpreted as a command by device." << std::endl;
        exception(MALICIOUS_SEQUENCE_IN_RAW_DATA);
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
        if(send_IM.deliveryReport != driver->getIMDeliveryReport(notification.buffer))
        {
            std::cout << "Device did not receive a delivered acknowledgment for Instant Message: \"" << send_IM.buffer << "\"" << std::endl;
            RTT::log(RTT::Error) << "Device did not receive a delivered acknowledgment for Instant Message: \"" << send_IM.buffer << "\"" << std::endl;
        }
        if(!IM_notification_ack)
            IM_notification_ack = true;
        return ;
    }
    else if(notification.notification == CANCELED_IM)
    {
        std::cout << "Error sending Instant Message: \"" << send_IM.buffer << "\". Be sure to wait delivery of last IM." << std::endl;
        RTT::log(RTT::Error) << "Error sending Instant Message: \"" << send_IM.buffer << "\". Be sure to wait delivery of last IM." << std::endl;
        return ;
    }
    processParticularNotification(notification);
}

void Task::processParticularNotification(NotificationInfo const &notification)
{
    std::cout << "Notification NOT implemented: \"" << notification.buffer << "\"." << std::endl;
    RTT::log(RTT::Error) << "Notification NOT implemented: \"" << notification.buffer << "\"." << std::endl;
}
