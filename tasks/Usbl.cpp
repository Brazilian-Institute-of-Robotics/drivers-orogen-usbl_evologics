/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Usbl.hpp"
#include <usbl_evologics/DriverTypes.hpp>

using namespace usbl_evologics;

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

usbl_evologics::ReverseMode reverse_mode = usbl_evologics::REVERSE_POSITION_SENDER;

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Usbl.hpp for more detailed
// documentation about them.




bool Usbl::configureHook()
{
    
    if (! UsblBase::configureHook())
        return false;
    driver.open(_device_string.get(), reverse_mode);

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
    driver.setKeepOnline(_keep_online.get());
    driver.setPositionEnable(_position_enable.get());
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
    std::cout << "USBL orogen update hook\n";

    UsblBase::updateHook();
    usbl_evologics::SendInstantMessage send_im;
    if (driver.getInstantMessageDeliveryStatus() != usbl_evologics::PENDING) {

	if(driver.newPositionAvailable()){
		std::cout << "usbl orogen: new pos avail. for sending to AUV!\n";
		driver.sendPositionToAUV();
	}
	else if (_message_input.read(send_im) == RTT::NewData) {
            std::cout << "Send IM" << send_im.destination << std::endl;
            driver.sendInstantMessage(send_im);
        } 
    }
    //TODO write out the status of the InstantMessages
    std::string buffer;
    while (_burstdata_input.read(buffer) == RTT::NewData){
        driver.sendBurstData(reinterpret_cast<const uint8_t*>(buffer.c_str()), buffer.size());
    }
    _stats.write(driver.getDeviceStatus());
    _connection_status.write(driver.getConnectionStatus());
    while (driver.hasPacket()){
        uint8_t buffer[2000];
        if (size_t len = driver.read(buffer, 2000)){
            char const* buffer_as_string = reinterpret_cast<char const*>(buffer);
            std::string s = std::string(buffer_as_string, len);
            _burstdata_output.write(s);
        }
    }
    while (driver.getInboxSize()){
        _message_output.write(driver.dropInstantMessage());
        std::cout << "cancel IM" << std::endl;
        writeOutPosition();
    }
    std::cout << "write out position" << std::endl;
    writeOutPosition();
    std::cout << "write out position end" << std::endl;
}

void Usbl::writeOutPosition(){
    usbl_evologics::Position pos = driver.getPosition(false);
    base::samples::RigidBodyState rbs;
    std::cout << "Position vorm Rausschreiben" << std::endl;
    std::cout << "X: " << pos.x << std::endl;
    std::cout << "Y: " << pos.y << std::endl;
    std::cout << "Z: " << pos.z << std::endl;
    rbs.position(0) = pos.x;
    rbs.position(1) = pos.y;
    rbs.position(2) = pos.z;
    rbs.time = pos.time;
    rbs.cov_position = pow(pos.accouracy, 2.0) * base::Matrix3d::Identity();
    _position_samples.write(rbs);
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
void Usbl::storePermanently()
{
    std::cout <<"Store Settings permanently on device" << std::endl;
    driver.storeSettings();

}
