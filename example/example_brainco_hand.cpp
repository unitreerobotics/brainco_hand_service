#include <unitree/dds_wrapper/robots/go2/go2.h>

int main(int argc, char** argv)
{
    unitree::robot::ChannelFactory::Instance()->Init(0, "");

    std::string ns = argc > 1 ? argv[1] : "left";
    auto lowcmd = std::make_unique<unitree::robot::go2::publisher::MotorCmds>("rt/brainco/"+ns+"/cmd", 6);
    for(auto & finger : lowcmd->msg_.cmds())
    {
        finger.dq() = 1.; // max speed
    }
    auto lowstate = std::make_shared<unitree::robot::go2::subscription::MotorStates>("rt/brainco/"+ns+"/state", 6);
    lowstate->wait_for_connection();

    std::array<uint16_t, 6> positions;

    auto hand_ctrl = [&](std::array<uint16_t, 6> & pos) {
        for(int i(0); i<6; i++) {
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