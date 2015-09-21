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
