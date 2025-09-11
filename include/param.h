#pragma once

#include <stdint.h>
#include <chrono>
#include <iostream>
#include <boost/program_options.hpp>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <iomanip>

/* ---------- logger ---------- */
namespace spdlog
{
inline void create_logger(std::string log_path)
{
    // 创建控制台日志输出（带颜色）
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // 创建文件日志输出
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path, true);

    // 将两个 sinks 组合为一个多目标日志器
    std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};
    auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());

    // 设置日志格式
    logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

    // 设置日志刷新方式
    logger->flush_on(spdlog::level::info);

    // 将 logger 设置为全局默认日志器（可选）
    spdlog::set_default_logger(logger);
}

inline std::string create_log_name()
{
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_c);

    // 格式化字符串
    std::ostringstream oss;
    oss << "log_"
        << std::put_time(&now_tm, "%Y-%m-%d_%H-%M-%S") // 格式化：年-月-日_时-分-秒
        << ".log";
    return oss.str();
}

} // namespace spdlog


namespace param
{
/* ---------- 全局参数声明 ---------- */
inline std::string VERSION = "1.0.0.0";
inline std::filesystem::path bin_path; // 本程序的路径
inline std::filesystem::path proj_dir; // 工程路径
inline std::filesystem::path config_dir; // 本程序的路径
inline YAML::Node config; // config.yaml的内容 default_config 可以没有
// 本程序的参数

// 获取执行程序的路径
inline std::filesystem::path get_bin_path() {
    std::vector<char> path(1024);
    ssize_t len = readlink("/proc/self/exe", &path[0], path.size());
    if (len != -1) {
        path[len] = '\0';  // Null-terminate the result
        return std::filesystem::path(&path[0]);
    } else {
        spdlog::error("Failed to get executable path.");
        exit(1);
    }
}

/* ---------- config.yaml ---------- */
void load_config_file()
{
    assert(std::filesystem::exists(bin_path)); // 需先执行 param::helper
    if(bin_path.parent_path().filename() == "bin" || bin_path.parent_path().filename() == "build")
    {
        proj_dir = bin_path.parent_path().parent_path();
        config_dir = proj_dir / "config";
    }
    else
    {
        proj_dir = bin_path.parent_path();
        config_dir = proj_dir;
    }

    try {
        std::string config_file = (config_dir / "config.yaml").string();
        if(std::filesystem::exists(config_file))
        {
            config = YAML::LoadFile(config_file);
        }
    } catch (const YAML::BadFile& e) {
        spdlog::error("Failed to load config.yaml: {}", e.what());
        exit(1);
    }
}

/* ---------- 命令行参数 ---------- */
namespace po = boost::program_options;

//※ 本函数必须在main函数中调用
po::variables_map helper(int argc, char** argv) 
{
    bin_path = get_bin_path();
    load_config_file(); // 直接加载配置文件

    po::options_description desc("Brainco Hand Service");
    desc.add_options()
        ("help,h", "produce help message")
        ("version,v", "show version")
        ("log", "record log file")
        ("network_interface,n", po::value<std::string>()->default_value(""), "dds network interface")
        ("serial,s", po::value<std::string>()->required(), "serial port name")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        exit(0);
    }
    if (vm.count("version"))
    {
        std::cout << "Version: " << VERSION << std::endl;
        exit(0);
    }

    // 设置日志级别
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#else
    spdlog::set_level(spdlog::level::info);
#endif

    if(vm.count("log"))
    {
        std::filesystem::create_directories(proj_dir / "log");
        auto log_path = proj_dir.string() + "/log/" + spdlog::create_log_name();
        spdlog::create_logger(log_path);
    }
    return vm;
}

}