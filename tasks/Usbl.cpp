/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Usbl.hpp"
#include <usbl_evologics/DriverTypes.hpp>

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
    driver.open(_device_string.get());

    driver.setCarrierWaveformId(_carrier_waveform_id.get());
    driver.setClusterSize(_cluster_size.get());
    driver.setIdleTimeout(_idle_timeout.get());
    driver.setImRetry(_im_retry.get());
    driver.setLocalAddress(_local_address.get());
    driver.setLowGain(_low_gain.get());
    driver.setPacketTime(_packet_time.get());
    driver.setRemoteAddress(_remote_address.get());
    driver.setRetryCount(_retry_count.get());
    driver.setRetryTimeout(_retry_timeout.get());
    driver.setSourceLevel(_source_level.get());
    driver.setSourceLevelControl(_source_level_control.get());
    driver.setSpeedSound(_sound_speed.get());


    
    
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
    usbl_evologics::SendInstantMessage send_im;
    while (_message_input.read(send_im) == RTT::NewData){
        driver.sendInstantMessage(&send_im);
    }
    std::string buffer;
    while (_burstdata_input.read(buffer) == RTT::NewData){
        driver.sendBurstData(reinterpret_cast<const uint8_t*>(buffer.c_str()), buffer.size());
    }
    _stats.write(driver.getDeviceStatus());
    _connection_status.write(driver.getConnectionStatus());
    while (driver.hasPacket()){
        uint8_t buffer[100];
        if (size_t len = driver.read(buffer, 100)){
            char const* buffer_as_string = reinterpret_cast<char const*>(buffer);
            std::string s = std::string(buffer_as_string, len);
            _burstdata_output.write(s);
        }
    }
    while (driver.getInboxSize()){
        _message_output.write(driver.dropInstantMessage());
        writeOutPosition();
    }
}

void Usbl::writeOutPosition(){
    usbl_evologics::Position pos = driver.getPosition(false);
    base::samples::RigidBodyState rbs;
    rbs.position(0) = pos.x;
    rbs.position(1) = pos.y;
    rbs.position(2) = pos.z;
    rbs.time = base::Time::now();
    _position_sample.write(rbs);
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
bool Usbl::storePermanently()
{
    std::cout <<"Store Settings permanently on device" << std::endl;
    driver.storeSettings();

}
