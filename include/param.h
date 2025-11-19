#pragma once

#include <stdint.h>
#include <chrono>
#include <iostream>
#include <boost/program_options.hpp>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <memory>
#include <iomanip>


namespace param
{
    inline std::string VERSION = "1.1.0.0";
    namespace po = boost::program_options;


    po::variables_map helper(int argc, char** argv) 
    {
        po::options_description desc("Unitree Brainco Hand Service");
        desc.add_options()
            ("help,h", "produce help message")
            ("version,v", "show version")
            ("network_interface,n", po::value<std::string>()->default_value(""), "dds network interface");

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


    #ifndef NDEBUG
        spdlog::set_level(spdlog::level::debug);
    #else
        spdlog::set_level(spdlog::level::info);
    #endif

        return vm;
    }


}