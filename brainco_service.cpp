#include "stark-sdk.h"
#include "param.h"

#include "dds/Publisher.h"
#include "dds/Subscription.h"
#include <unitree/idl/go2/MotorCmds_.hpp>
#include <unitree/idl/go2/MotorStates_.hpp>
#include <unitree/common/thread/recurrent_thread.hpp>

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
    init_cfg(StarkHardwareType::STARK_HARDWARE_TYPE_REVO2_BASIC, StarkProtocolType::STARK_PROTOCOL_TYPE_MODBUS, LogLevel::LOG_LEVEL_WARN, 1024);

    auto port_name = vm["serial"].as<std::string>();
    if (port_name.empty()) {
        spdlog::critical("Serial port name is empty. Please specify a valid serial port.");
        exit(1);
    }

    DeviceHandler* handle = nullptr;
    DeviceInfo* info = nullptr;

    if (slave_id == 126) {
        handle = modbus_open(port_name.c_str(), baudrate);
        if (handle != nullptr) {
            modbus_set_finger_unit_mode(handle, L_id, FINGER_UNIT_MODE_NORMALIZED);
        }
        info = modbus_get_device_info(handle, L_id);
        if(info == nullptr) {
            spdlog::critical("Failed to connect left hand. Id: {}", slave_id);
            exit(1);
        }
        spdlog::info("Slave id: {} SKU Type: {}, Serial Number: {}, Firmware Version: {}\n", slave_id, (uint8_t)info->sku_type, info->serial_number, info->firmware_version);
    }
    else if (slave_id == 127) {
        handle = modbus_open(port_name.c_str(), baudrate);
        if (handle != nullptr) {
            modbus_set_finger_unit_mode(handle, R_id, FINGER_UNIT_MODE_NORMALIZED);
        }
        info = modbus_get_device_info(handle, R_id);
        if(info == nullptr) {
            spdlog::critical("Failed to connect right hand. Id: {}", slave_id);
            exit(1);
        }
        spdlog::info("Slave id: {} SKU Type: {}, Serial Number: {}, Firmware Version: {}\n", slave_id, (uint8_t)info->sku_type, info->serial_number, info->firmware_version);   
    }
    else {
        spdlog::critical("Invalid slave id: {}. Only 126 (left) and 127 (right) are supported.", slave_id);
        exit(1);
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

    signal(SIGINT, signal_handler);

    while (running)
    {
        auto start_time = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < 6; i++)
        {
            positions[i] = std::clamp(lowcmd->msg_.cmds()[i].q(), 0.f, 1.f) * 1000;
            speeds[i] = std::clamp(lowcmd->msg_.cmds()[i].dq(), 0.f, 1.f) * 1000;
        }

        modbus_set_finger_positions_and_speeds(handle, slave_id, positions, speeds, 6);
        auto finger_status = modbus_get_motor_status(handle, slave_id);

        if (finger_status != nullptr) {
            for (int i = 0; i < 6; i++) {
                lowstate->msg_.states()[i].q() = finger_status->positions[i] / 1000.f;
                lowstate->msg_.states()[i].dq() = finger_status->speeds[i] / 1000.f;
            }
            lowstate->unlockAndPublish();
        }

        // 1000 Hz loop rate
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        int sleep_time = 1000.0 - elapsed;
        if (sleep_time > 0)
            std::this_thread::sleep_for(std::chrono::microseconds(sleep_time));
        free_motor_status_data(finger_status);
    }

    
    return 0;
}