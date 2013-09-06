/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "BurstDataProducer.hpp"

using namespace usbl_orogen;

BurstDataProducer::BurstDataProducer(std::string const& name, TaskCore::TaskState initial_state)
    : BurstDataProducerBase(name, initial_state)
{
}

BurstDataProducer::BurstDataProducer(std::string const& name, RTT::ExecutionEngine* engine, TaskCore::TaskState initial_state)
    : BurstDataProducerBase(name, engine, initial_state)
{
}

BurstDataProducer::~BurstDataProducer()
{
}



/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See BurstDataProducer.hpp for more detailed
// documentation about them.




bool BurstDataProducer::configureHook()
{
    
    if (! BurstDataProducerBase::configureHook())
        return false;
    

    

    
    return true;
    
}



bool BurstDataProducer::startHook()
{
    
    if (! BurstDataProducerBase::startHook())
        return false;
    

    

    
    return true;
    
}



void BurstDataProducer::updateHook()
{
    
    BurstDataProducerBase::updateHook();
    _burstdata_output.write(_message_content.get());
}



void BurstDataProducer::errorHook()
{
    
    BurstDataProducerBase::errorHook();
    

    

    
}



void BurstDataProducer::stopHook()
{
    
    BurstDataProducerBase::stopHook();
    

    

    
}



void BurstDataProducer::cleanupHook()
{
    
    BurstDataProducerBase::cleanupHook();
    

    

    
}

