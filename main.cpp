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
#include <filesystem>
#include <cstdlib>

std::atomic<bool> running(true);
void signal_handler(int) { running = false; }

// Brainco Hand ID
constexpr uint8_t L_id = 0x7e;
constexpr uint8_t R_id = 0x7f;
constexpr uint32_t baudrate = 460800;
constexpr int kMaxConsecutiveCommFailures = 50;

// ------------------ Utility ------------------
std::vector<std::string> getAvailableSerialPorts() {
    std::vector<std::string> ports;
    for (const auto& entry : std::filesystem::directory_iterator("/dev")) {
        std::string path = entry.path().string();

        if (path.rfind("/dev/ttyUSB", 0) == 0) ports.push_back(path);
        if (path.rfind("/dev/ttyHAND", 0) == 0) ports.push_back(path);
        if (path.rfind("/dev/ttyUN", 0) == 0) ports.push_back(path);
    }
    spdlog::info("Available Serial Ports: {}", fmt::join(ports, ", "));
    return ports;
}


// Hand connection struct
struct HandConnection {
    DeviceHandler* handle{nullptr};
    CDeviceInfo* info{nullptr};
    std::string port;
};

void close_hand_connection(HandConnection& conn) {
    if (conn.handle) {
        modbus_close(conn.handle);
        conn.handle = nullptr;
    }
    if (conn.info) {
        free_device_info(conn.info);
        conn.info = nullptr;
    }
}

// Try connecting a hand on a given port
HandConnection try_connect_hand(const std::string& port, uint8_t slave_id) {
    HandConnection ret;
    ret.port = port;

    DeviceHandler* handle = modbus_open(port.c_str(), baudrate);
    if (!handle) {
        spdlog::warn("Failed to open {} at {} baud", port, baudrate);
        return ret;
    }

    CDeviceInfo* info = stark_get_device_info(handle, slave_id);
    if (!info) {
        spdlog::warn("Failed to get device info from {} port", port);
        modbus_close(handle);
        return ret;
    }

    stark_set_finger_unit_mode(handle, slave_id, FINGER_UNIT_MODE_NORMALIZED);

    ret.handle = handle;
    ret.info = info;
    return ret;
}

// Find a hand from available ports given allowed SKUs
HandConnection find_hand(std::vector<std::string>& ports, uint8_t slave_id, const std::vector<SkuType>& allowed_skus, const std::string& hand_name) {
    for (const auto& port : ports) {
        spdlog::info("Trying port for {} hand from {} port", hand_name, port);
        auto conn = try_connect_hand(port, slave_id);
        if (!conn.handle) continue;

        if (std::find(allowed_skus.begin(), allowed_skus.end(), conn.info->sku_type) != allowed_skus.end()) {
            spdlog::info("{} hand bound to {} port", hand_name, port);
            ports.erase(std::remove(ports.begin(), ports.end(), port), ports.end());
            return conn;
        } else {
            int sku_type = static_cast<int>(conn.info->sku_type);
            modbus_close(conn.handle);
            free_device_info(conn.info);
            spdlog::warn("Port {} is not {} hand (sku {}). Closed.", port, hand_name, sku_type);
        }
    }
    return {};
}

bool has_required_hands(const HandConnection& left_conn, const HandConnection& right_conn) {
    return left_conn.handle && right_conn.handle;
}

void log_missing_hands(const HandConnection& left_conn, const HandConnection& right_conn) {
    if (!left_conn.handle) {
        spdlog::error("Missing left Brainco hand.");
    }
    if (!right_conn.handle) {
        spdlog::error("Missing right Brainco hand.");
    }
}

// ------------------ Hand Update Loop ------------------
bool update_finger(DeviceHandler* handle, uint8_t slave_id,
                   unitree::robot::SubscriptionBase<unitree_go::msg::dds_::MotorCmds_>* lowcmd,
                   unitree::robot::RealTimePublisher<unitree_go::msg::dds_::MotorStates_>* lowstate,
                   const std::string& ns) {
    uint16_t positions[6], speeds[6];

    for (int i = 0; i < 6; ++i) {
        positions[i] = static_cast<uint16_t>(std::clamp(lowcmd->msg_.cmds()[i].q(), 0.f, 1.f) * 1000.f);
        speeds[i]    = static_cast<uint16_t>(std::clamp(lowcmd->msg_.cmds()[i].dq(), 0.f, 1.f) * 1000.f);
    }

    // Write commands
    stark_set_finger_positions_and_speeds(handle, slave_id, positions, speeds, 6);

    // Read status
    auto status = stark_get_motor_status(handle, slave_id);
    if (!status) return false;

    for (int i = 0; i < 6; ++i) {
        lowstate->msg_.states()[i].q()        = status->positions[i] / 1000.f;
        lowstate->msg_.states()[i].dq()       = status->speeds[i] / 1000.f;
        lowstate->msg_.states()[i].tau_est()  = status->currents[i] / 1000.f;

        // if (status->currents[i] > 800) {
        //     spdlog::warn("{} finger {} over current: {} mA", ns, i, status->currents[i]);
        // }
    }
    lowstate->unlockAndPublish();
    free_motor_status_data(status);
    return true;
}

// Worker thread for each hand
void hand_worker(DeviceHandler* handle, uint8_t slave_id, const std::string& ns) {
    spdlog::info("🚀 Starting worker for {} (slave {})", ns, (int)slave_id);

    // DDS setup
    auto lowcmd   = std::make_shared<unitree::robot::SubscriptionBase<unitree_go::msg::dds_::MotorCmds_>>("rt/brainco/" + ns + "/cmd");
    lowcmd->msg_.cmds().resize(6);
    for (auto& finger : lowcmd->msg_.cmds()) finger.dq() = 1.;

    auto lowstate = std::make_unique<unitree::robot::RealTimePublisher<unitree_go::msg::dds_::MotorStates_>>("rt/brainco/" + ns + "/state");
    lowstate->msg_.states().resize(6);

    int consecutive_comm_failures = 0;
    while (running) {
        auto start_time = std::chrono::high_resolution_clock::now();
        if (!update_finger(handle, slave_id, lowcmd.get(), lowstate.get(), ns)) {
            ++consecutive_comm_failures;
            if (consecutive_comm_failures == 1 || consecutive_comm_failures % 10 == 0) {
                spdlog::warn("{} Brainco hand communication failure count: {}", ns, consecutive_comm_failures);
            }
            if (consecutive_comm_failures >= kMaxConsecutiveCommFailures) {
                spdlog::error("{} Brainco hand appears offline. Exiting for systemd restart.", ns);
                std::exit(1);
            }
        } else {
            consecutive_comm_failures = 0;
        }
        auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
        int sleep_us = 10000 - static_cast<int>(elapsed_us); // 100Hz
        if (sleep_us > 0) std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));
    }

    spdlog::info("Worker for {} exiting (closing handle)", ns);
    if (handle) modbus_close(handle);
}

int main(int argc, char** argv) {
    signal(SIGINT, signal_handler);

    auto vm = param::helper(argc, argv);
    unitree::robot::ChannelFactory::Instance()->Init(0, vm["network_interface"].as<std::string>());

    init_logging(LogLevel::LOG_LEVEL_ERROR);

    std::vector<std::string> available_ports = getAvailableSerialPorts();
    if (available_ports.empty()) {
        spdlog::error("No ttyUSB serial ports found. Exiting for systemd restart.");
        return 1;
    }

    HandConnection left_conn  = find_hand(available_ports, L_id, {SkuType::SKU_TYPE_SMALL_LEFT, SkuType::SKU_TYPE_MEDIUM_LEFT}, "left");
    HandConnection right_conn = find_hand(available_ports, R_id, {SkuType::SKU_TYPE_SMALL_RIGHT, SkuType::SKU_TYPE_MEDIUM_RIGHT}, "right");

    if (!has_required_hands(left_conn, right_conn)) {
        log_missing_hands(left_conn, right_conn);
        spdlog::error("Both left and right Brainco hands must be online. Exiting for systemd restart.");
        close_hand_connection(left_conn);
        close_hand_connection(right_conn);
        return 1;
    }

    std::thread left_thread, right_thread;
    left_thread  = std::thread(hand_worker, left_conn.handle, L_id, "left");
    right_thread = std::thread(hand_worker, right_conn.handle, R_id, "right");

    if (left_thread.joinable())  left_thread.join();
    if (right_thread.joinable()) right_thread.join();

    if (left_conn.info)  free_device_info(left_conn.info);
    if (right_conn.info) free_device_info(right_conn.info);

    spdlog::info("exit.");
    return 0;
}