#include "dds/Publisher.h"
#include "dds/Subscription.h"
#include <unitree/idl/go2/MotorCmds_.hpp>
#include <unitree/idl/go2/MotorStates_.hpp>

int main(int argc, char** argv)
{
    unitree::robot::ChannelFactory::Instance()->Init(0, "");

    std::string ns = argc > 1 ? argv[1] : "left";
    auto lowcmd = std::make_unique<unitree::robot::RealTimePublisher<unitree_go::msg::dds_::MotorCmds_>>("rt/brainco/"+ns+"/cmd");
    lowcmd->msg_.cmds().resize(6);
    for(auto & finger : lowcmd->msg_.cmds())
    {
        finger.dq() = 1.; // max speed
    }
    auto lowstate = std::make_shared<unitree::robot::SubscriptionBase<unitree_go::msg::dds_::MotorStates_>>("rt/brainco/"+ns+"/state");
    lowstate->wait_for_connection();

    std::array<uint16_t, 6> positions;

    auto hand_ctrl = [&](std::array<uint16_t, 6> & pos)
    {
        for(int i(0); i<6; i++)
        {
            lowcmd->msg_.cmds()[i].q() = pos[i];
        }
        lowcmd->unlockAndPublish();
    };

    while ( true)
    {
        positions = {0, 0, 0, 0, 0, 0};
        hand_ctrl(positions);
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
        positions = {0, 1, 1, 1, 1, 1};
        hand_ctrl(positions);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        positions = {1, 1, 1, 1, 1, 1};
        hand_ctrl(positions);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    
    return 0;
}