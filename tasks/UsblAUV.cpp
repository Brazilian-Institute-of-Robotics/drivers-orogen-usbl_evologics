/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "UsblAUV.hpp"

using namespace usbl_evologics;

UsblAUV::UsblAUV(std::string const& name)
    : UsblAUVBase(name)
{
}

UsblAUV::UsblAUV(std::string const& name, RTT::ExecutionEngine* engine)
    : UsblAUVBase(name, engine)
{
}

UsblAUV::~UsblAUV()
{
}



/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See UsblAUV.hpp for more detailed
// documentation about them.

bool UsblAUV::configureHook()
{
    if (! UsblAUVBase::configureHook())
        return false;

    return true;
}
bool UsblAUV::startHook()
{
    if (! UsblAUVBase::startHook())
        return false;
    return true;
}
void UsblAUV::updateHook()
{
    UsblAUVBase::updateHook();
}
void UsblAUV::errorHook()
{
    UsblAUVBase::errorHook();
}
void UsblAUV::stopHook()
{
    UsblAUVBase::stopHook();
}
void UsblAUV::cleanupHook()
{
    UsblAUVBase::cleanupHook();
}
