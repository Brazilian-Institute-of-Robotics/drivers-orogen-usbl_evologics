/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "UsblDock.hpp"

using namespace usbl_evologics;

UsblDock::UsblDock(std::string const& name)
    : UsblDockBase(name)
{
}

UsblDock::UsblDock(std::string const& name, RTT::ExecutionEngine* engine)
    : UsblDockBase(name, engine)
{
}

UsblDock::~UsblDock()
{
}



/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See UsblDock.hpp for more detailed
// documentation about them.

bool UsblDock::configureHook()
{
    if (! UsblDockBase::configureHook())
        return false;

    // Just to be sure the usbl will output position samples.
    if(!driver->getPositioningDataOutput())
        driver->setPositioningDataOutput(true);

    return true;
}
bool UsblDock::startHook()
{
    if (! UsblDockBase::startHook())
        return false;
    return true;
}
void UsblDock::updateHook()
{
    UsblDockBase::updateHook();
}
void UsblDock::errorHook()
{
    UsblDockBase::errorHook();
}
void UsblDock::stopHook()
{
    UsblDockBase::stopHook();
}
void UsblDock::cleanupHook()
{
    UsblDockBase::cleanupHook();
}
void UsblDock::processParticularNotification(NotificationInfo const &notification)
{
    if(notification.notification == USBLLONG)
    {   base::samples::RigidBodyState pose = driver->getPose(driver->getPose(notification.buffer));
        pose.sourceFrame = _source_frame.get();
        pose.targetFrame = _target_frame.get();
        _position_samples.write(pose);
        return ;
    }
    else if(notification.notification == USBLANGLE)
    {
        _direction_samples.write(driver->getDirection(notification.buffer));
        return ;
    }
    else
    {
        RTT::log(RTT::Error) << "Usbl_evologics UsblDock.cpp. Notification NOT implemented: \"" << UsblParser::printBuffer(notification.buffer) << "\"." << std::endl;
        return ;
    }
}
