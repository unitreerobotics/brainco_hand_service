#include "stark-sdk.h"
#include "param.h"

#include "dds/Publisher.h"
#include "dds/Subscription.h"
#include <unitree/idl/go2/MotorCmds_.hpp>
#include <unitree/idl/go2/MotorStates_.hpp>
#include <unitree/common/thread/recurrent_thread.hpp>

// Brainco Hand ID
constexpr uint8_t L_id = 0x7e;
constexpr uint8_t R_id = 0x7f;
constexpr uint32_t baudrate = 460800;

int main(int argc, char** argv)
{
    /* parser command line arguments */
    auto vm = param::helper(argc, argv);

    unitree::robot::ChannelFactory::Instance()->Init(0, vm["network_interface"].as<std::string>());

    uint8_t slave_id = vm["id"].as<int>();

    /* Config Hand */
    init_cfg(StarkFirmwareType::STARK_FIRMWARE_TYPE_V2_STANDARD, StarkProtocolType::STARK_PROTOCOL_TYPE_MODBUS, LogLevel::LOG_LEVEL_INFO);
    
    auto port_name = vm["serial"].as<std::string>();
    auto handle = modbus_open(port_name.c_str(), baudrate, slave_id);
    modbus_set_finger_unit_mode(handle, slave_id, FINGER_UNIT_MODE_NORMALIZED);

    // check connection
    auto info = modbus_get_device_info(handle, slave_id);

    if(NULL == info) {
        spdlog::critical("Failed to connect hand. Id: {}", slave_id);
        exit(1);
    } else {
        spdlog::info("Slave id: {} SKU Type: {}, Serial Number: {}, Firmware Version: {}\n", slave_id, (uint8_t)info->sku_type, info->serial_number, info->firmware_version);
    }
    std::string ns = "";
    if(info->sku_type == SkuType::SKU_TYPE_SMALL_LEFT) {
        ns = "left";
    } else if(info->sku_type == SkuType::SKU_TYPE_SMALL_RIGHT) {
        ns = "right";
    } else if(info->sku_type == SkuType::SKU_TYPE_MEDIUM_LEFT) {
        ns = "left";
    } else if(info->sku_type == SkuType::SKU_TYPE_MEDIUM_RIGHT) {
        ns = "right";
    }else{
        spdlog::error("Hand type not supported.");
        return -1;
    }

    /* Config dds */
    auto lowcmd = std::make_shared<unitree::robot::SubscriptionBase<unitree_go::msg::dds_::MotorCmds_>>("rt/brainco/"+ns+"/cmd");
    lowcmd->msg_.cmds().resize(6);
    for(auto & finger : lowcmd->msg_.cmds())
    {
        finger.dq() = 1.; // max speed
    }
    auto lowstate = std::make_unique<unitree::robot::RealTimePublisher<unitree_go::msg::dds_::MotorStates_>>("rt/brainco/"+ns+"/state");
    lowstate->msg_.states().resize(6);

    uint16_t positions[6];
    uint16_t speeds[6];
    while (true)
    {
        for(int i(0); i<6; i++)
        {
            positions[i] = std::clamp(lowcmd->msg_.cmds()[i].q(), 0.f, 1.f) * 1000;
            speeds[i] = std::clamp(lowcmd->msg_.cmds()[i].dq(), 0.f, 1.f) * 1000;
        }

        modbus_set_finger_positions_and_speeds(handle, slave_id, positions, speeds, 6);
        auto finger_status = modbus_get_motor_status(handle, slave_id);
        for(int i(0); i<6; i++)
        {
            lowstate->msg_.states()[i].q() = finger_status->positions[i] / 1000.f;
            lowstate->msg_.states()[i].dq() = finger_status->speeds[i] / 1000.f;
        }
        lowstate->unlockAndPublish();
    }
    
    return 0;
}