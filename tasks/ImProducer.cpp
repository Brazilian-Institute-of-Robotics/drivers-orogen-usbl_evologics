/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "ImProducer.hpp"
#include <usbl_evologics/DriverTypes.hpp>

using namespace usbl_orogen;

ImProducer::ImProducer(std::string const& name, TaskCore::TaskState initial_state)
    : ImProducerBase(name, initial_state)
{
}

ImProducer::ImProducer(std::string const& name, RTT::ExecutionEngine* engine, TaskCore::TaskState initial_state)
    : ImProducerBase(name, engine, initial_state)
{
}

ImProducer::~ImProducer()
{
}



/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See ImProducer.hpp for more detailed
// documentation about them.




bool ImProducer::configureHook()
{
    
    if (! ImProducerBase::configureHook())
        return false;
    

    

    
    return true;
    
}



bool ImProducer::startHook()
{
    
    if (! ImProducerBase::startHook())
        return false;
    

    

    
    return true;
    
}



void ImProducer::updateHook()
{
    
    ImProducerBase::updateHook();
    usbl_evologics::SendInstantMessage im;
    im.destination = _destination.get();
    im.deliveryReport = _delivery_report.get();
    im.deliveryStatus = usbl_evologics::PENDING; 
    im.buffer.resize(_message_content.get().size());
    for (int i = 0; i<_message_content.get().size(); i++){
        im.buffer[i] = _message_content.get().at(i);
    }
    //im.len = _message_content.get().size();
    _im_output.write(im);
}



void ImProducer::errorHook()
{
    
    ImProducerBase::errorHook();
    

    

    
}



void ImProducer::stopHook()
{
    
    ImProducerBase::stopHook();
    

    

    
}



void ImProducer::cleanupHook()
{
    
    ImProducerBase::cleanupHook();
    

    

    
}

