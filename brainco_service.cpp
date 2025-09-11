#include "stark-sdk.h"
#include "param.h"

#include <unitree/dds_wrapper/robots/go2/go2.h>
#include <unitree/common/thread/recurrent_thread.hpp>
#include <unitree/idl/hg/PressSensorState_.hpp>

#include <iostream>
#include <csignal>
#include <chrono>
#include <thread>
#include <algorithm>
#include <atomic>

std::atomic<bool> running(true);
void signal_handler(int signum)
{
    std::cout << "\nCtrl+C detected. Exiting loop..." << std::endl;
    running = false;
}

int main(int argc, char** argv)
{
    /* parser command line arguments */
    auto vm = param::helper(argc, argv);
    std::string ns = "";

    unitree::robot::ChannelFactory::Instance()->Init(0, vm["network_interface"].as<std::string>());
    std::unique_ptr<unitree::robot::RealTimePublisher<unitree_hg::msg::dds_::PressSensorState_>> touch_sensor = nullptr;

    /* Config Hand */
    init_cfg(STARK_PROTOCOL_TYPE_MODBUS, LOG_LEVEL_DEBUG);
    auto port_name = vm["serial"].as<const char *>();
    auto cfg = auto_detect_modbus_revo2(port_name, true);
    if (!cfg) {
        spdlog::critical("Failed to auto-detect Modbus device configuration on port: {}", port_name);
        return -1;
    }
    uint8_t slave_id = cfg->slave_id;
    auto handle = modbus_open(cfg->port_name, cfg->baudrate);
    free_device_config(cfg);

    auto info = stark_get_device_info(handle, slave_id);
    if (info != NULL)
    {
        spdlog::info("Slave id: {} SKU Type: {}, Serial Number: {}, Firmware Version: {}\n", \
            slave_id, (uint8_t)info->sku_type, info->serial_number, info->firmware_version);

        // Config namespace based on hand type
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
        
        if (info->hardware_type == STARK_HARDWARE_TYPE_REVO1_TOUCH || info->hardware_type == STARK_HARDWARE_TYPE_REVO2_TOUCH)
        {
            // 启用全部触觉传感器
            stark_enable_touch_sensor(handle, slave_id, 0x1F);
            usleep(1000 * 1000); // wait for touch sensor to be ready
            touch_sensor = std::make_unique<unitree::robot::RealTimePublisher<unitree_hg::msg::dds_::PressSensorState_>>("rt/brainco/"+ns+"/touch");
        }
    }
    else
    {
        spdlog::critical("Failed to connect hand. Id: {}", slave_id);
        return -1;
    }

    free_device_info(info);

    uint16_t positions[6];
    uint16_t speeds[6];

    signal(SIGINT, signal_handler);


    /* Config dds */
    auto lowcmd = std::make_shared<unitree::robot::go2::subscription::MotorCmds>("rt/brainco/"+ns+"/cmd", 6);
    for(auto & finger : lowcmd->msg_.cmds())
    {
        finger.dq() = 1.; // max speed
    }
    auto lowstate = std::make_unique<unitree::robot::go2::publisher::MotorStates>("rt/brainco/"+ns+"/state", 6);

    while (running)
    {
        for (int i = 0; i < 6; i++)
        {
            positions[i] = std::clamp(lowcmd->msg_.cmds()[i].q(), 0.f, 1.f) * 1000;
            speeds[i] = std::clamp(lowcmd->msg_.cmds()[i].dq(), 0.f, 1.f) * 1000;
        }

        stark_set_finger_positions_and_speeds(handle, slave_id, positions, speeds, 6);

        // Get motor status
        auto motor_status = stark_get_motor_status(handle, slave_id);

        if (motor_status != nullptr) {
            for (int i = 0; i < 6; i++) {
                lowstate->msg_.states()[i].q() = motor_status->positions[i] / 1000.f;
                lowstate->msg_.states()[i].dq() = motor_status->speeds[i] / 1000.f;
                lowstate->msg_.states()[i].tau_est() = motor_status->currents[i] / 1000.f;
                lowstate->msg_.states()[i].reserve()[0] = motor_status->states[i];
            }
            lowstate->unlockAndPublish();
        }
        free_motor_status_data(motor_status);

        // Get touch status if applicable
        if (touch_sensor != nullptr)
        {
            auto touch_status = stark_get_touch_status(handle, slave_id);

            if (touch_status != nullptr) {
                for(int i(0); i<5; i++)
                {
                    auto & item = touch_status->items[i];
                    if(item.status != 0) continue; // invalid

                    // Attention: PressSensorState_ message is not compatible with TouchFingerItem struct
                    // we just use its data fields to store the values
                    touch_sensor->msg_.pressure()[i] = item.normal_force1 / 100.f; // Range [0, 25]
                    touch_sensor->msg_.pressure()[i+5] = item.tangential_force1 / 100.f; // Range [0, 25]
                    touch_sensor->msg_.temperature()[i] = item.self_proximity1; // ?
                    touch_sensor->msg_.temperature()[i+5] = item.tangential_direction1; // 0 - 359; 0xFFFF means invalid

                }
                lowstate->unlockAndPublish();
            }
            free_touch_finger_data(touch_status);
        }

        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }

    
    return 0;
}