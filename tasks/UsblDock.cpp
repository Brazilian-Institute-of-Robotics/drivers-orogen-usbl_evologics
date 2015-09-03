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

    usbl = new Driver();
    usbl->setInterface(ETHERNET);

    usbl->openTCP(_ip_address.get(), _port.get());
    usbl->getCurrentSetting();
//  usbl->openTCP("localhost", 631);

    if(_change_parameters.get())
    {
        //TODO update parameters
    }

    mail_command = true;

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

    std::string out_msg;
    std::string raw_data_input;
    std::string raw_data_ouput;
    SendIM send_im;
    ReceiveIM receive_im;

    base::samples::RigidBodyState pose_sample;
    base::samples::RigidBodyState world_pose_sample;


    usbl->getConnetionStatus();


    if(_message_input.read(send_im) == RTT::NewData)
        usbl->sendInstantMessage(send_im);
    if(_rawdata_input.read(raw_data_input) == RTT::NewData)
        usbl->sendRawData(raw_data_input);

    // write/Read Core
    usbl->sendData(mail_command);
    if(usbl->readAnswer(mail_command, out_msg, raw_data_ouput))
    {
        if(out_msg == "NEW IM")
        {
            usbl->receiveInstantMessage(receive_im);
            _message_output.write(receive_im);
            std::cout << "Answer Command : "<< out_msg << std::endl;

        }
        if(out_msg == "NEW POSE")
        {
            usbl->getNewPose(pose_sample);
            _position_samples.write(pose_sample);

            if(_world_pose.connected())
            {
                if(_world_pose.read(world_pose_sample) == RTT::NewData)
                {
                    usbl->sendIMPose(world_pose_sample);
                }
            }
            else
                usbl->sendIMPose(pose_sample);
        }
        if(!raw_data_ouput.empty())
        {
            _rawdata_output.write(raw_data_ouput);
            std::cout << "raw data : " << raw_data_ouput << std::endl;
        }
    }

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
