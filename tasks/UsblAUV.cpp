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

    if (!_io_port.get().empty())
    {
        if(_io_port.get().find("serial") == std::string::npos)
        {
            RTT::log(RTT::Error) << "Usbl_evologics UsblAUV.cpp. WRONG INTERFACE, define serial connection in _io_port" << std::endl;
            exception(WRONG_INTERFACE);
            return false;
        }
    }

    temp_source_level = MINIMAL;

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

    base::samples::RigidBodyState pose_samples;
    // TODO need validation. Check if the source level is not determined by the source level of the remote device.
    while(!current_settings.sourceLevelControl && _position_samples.read(pose_samples) == RTT::NewData)
    {
        // In case the vehicle is near the surface and the sourceLevel is not set to be MINIMAL, it set this parameter to MINIMAL while auv is near the surface.
        if(pose_samples.position[2] > -0.5 && current_settings.sourceLevel != MINIMAL)
        {
            temp_source_level = current_settings.sourceLevel;
            current_settings.sourceLevel = MINIMAL;
            driver->setSourceLevel(current_settings.sourceLevel);
        }
        // Just to avoid a oscillatory movement around -0.5, set to a little bit deeper to set back the source level
        else if(pose_samples.position[2] < -1.0 && temp_source_level != MINIMAL)
        {
            current_settings.sourceLevel = temp_source_level;
            temp_source_level = MINIMAL;
            driver->setSourceLevel(current_settings.sourceLevel);
        }
    }
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

