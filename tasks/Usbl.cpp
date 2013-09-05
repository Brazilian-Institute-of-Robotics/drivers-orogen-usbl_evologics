/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Usbl.hpp"

using namespace usbl_orogen;

Usbl::Usbl(std::string const& name, TaskCore::TaskState initial_state)
    : UsblBase(name, initial_state)
{
}

Usbl::Usbl(std::string const& name, RTT::ExecutionEngine* engine, TaskCore::TaskState initial_state)
    : UsblBase(name, engine, initial_state)
{
}

Usbl::~Usbl()
{
}



/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Usbl.hpp for more detailed
// documentation about them.




bool Usbl::configureHook()
{
    
    if (! UsblBase::configureHook())
        return false;
    

    
    std::cout << _source_level.get() << std::endl;
    
    return true;
    
}



bool Usbl::startHook()
{
    
    if (! UsblBase::startHook())
        return false;
    

    

    
    return true;
    
}



void Usbl::updateHook()
{
    
    UsblBase::updateHook();
    

    

    
}



void Usbl::errorHook()
{
    
    UsblBase::errorHook();
    

    

    
}



void Usbl::stopHook()
{
    
    UsblBase::stopHook();
    

    

    
}



void Usbl::cleanupHook()
{
    
    UsblBase::cleanupHook();
    

    

    
}

