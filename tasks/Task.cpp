/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"

using namespace usbl_evologics;

Task::Task(std::string const& name)
    : TaskBase(name)
{
}

Task::Task(std::string const& name, RTT::ExecutionEngine* engine)
    : TaskBase(name, engine)
{
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

    DeviceSettings settings = _device_settings.get();
    DeviceSettings current_settings = driver->getCurrentSetting();
    if(_change_parameters.get())
    {
        //TODO need validation
        updateDeviceParameters(settings, current_settings);
    }

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
    return true;
}
void Task::updateHook()
{
    TaskBase::updateHook();
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
}
void Task::processIO()
{

    _device_settings.set(driver->getCurrentSetting());

    _acoustic_channel.write(getAcousticChannelparameters());

    AcousticConnection acoustic_connection = driver->getConnectionStatus();
    _acoustic_connection.write(acoustic_connection);

    // TODO define exactly what to do for each acoustic_connection status
    if(acoustic_connection.status == ONLINE || acoustic_connection.status == OFFLINE_READY || acoustic_connection.status == INITIATION_LISTEN )
    {
        if(_message_input.read(send_IM) == RTT::NewData)
            driver->sendInstantMessage(send_IM);

        iodrivers_base::RawPacket raw_data_input;
        if(_raw_data_input.read(raw_data_input) == RTT::NewData)
        {
            if(driver->getMode() == DATA)
                driver->sendRawData(raw_data_input.data);
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

    if(acoustic_connection.status == OFFLINE_ALARM)
        driver->resetDevice(DEVICE);

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

    if(!actual_setting.dropCount.empty() && !desired_setting.dropCount.empty())
    {
        // Only takes in account the first and actual channel
        // If desired value is 0, reset counter.
        if(desired_setting.dropCount.at(0) == 0)
            driver->resetDropCounter();
    }

    if(!actual_setting.overflowCounter.empty() && !desired_setting.overflowCounter.empty())
    {
        // Only takes in account the first and actual channel]
        // If desired value is 0, reset counter.
        if(desired_setting.overflowCounter.at(0) == 0)
            driver->resetOverflowCounter();
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
    return channel;
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
