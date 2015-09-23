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

    driver->getCurrentSetting();
    if(_change_parameters.get())
    {
        //TODO update parameters
    }

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


    try{
        Connection connection = driver->getConnectionStatus();
        _connection.write(connection);

        // TODO define exactly what to do for each connection status
        if(connection.status == ONLINE || connection.status == OFFLINE_READY || connection.status == INITIATION_LISTEN )
        {
            if(_message_input.read(send_IM) == RTT::NewData)
                driver->sendInstantMessage(send_IM);

            std::string raw_data_input;
            if(driver->getMode() == DATA && _rawdata_input.read(raw_data_input) == RTT::NewData)
                driver->sendRawData(raw_data_input);
        }

        while(driver->hasNotification())
            processNotification(driver->getNotification());
        while(driver->hasRawData())
            _rawdata_output.write(driver->getRawData());


        if(connection.status == OFFLINE_ALARM)
            driver->resetDevice(DEVICE);

    }
    catch (std::runtime_error &error){
        std::cout << "Error: "<< error.what() << std::endl;
        RTT::log(RTT::Error) << "Error: "<< error.what() << std::endl;

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
