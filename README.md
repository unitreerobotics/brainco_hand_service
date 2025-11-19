<div align="center">
  <h1 align="center">
    <a href="https://www.unitree.com" target="_blank">Brainco Hand Service</a>
  </h1>
  <p align="center">
    <a> English </a> | <a href="README_zh-CN.md">中文</a> </a>
  </p>
  <a href="https://www.unitree.com/" target="_blank">
    <img src="https://www.unitree.com/images/0079f8938336436e955ea3a98c4e1e59.svg" alt="Unitree LOGO" width="15%">
  </a>
</div>

# 0. 📖 Introduction

The G1 can be equipped with [BrainCoTech](https://www.brainco.cn)'s [Revo2 Dexterous Hand](https://www.brainco.cn/#/product/revo2), which features 6 degrees of freedom.

<p align="center">
  <a href="https://brainco-common-public.oss-cn-hangzhou.aliyuncs.com/web-config/docs-sdk/WbXwhniecMNLxKDj.webp">
    <img src="https://brainco-common-public.oss-cn-hangzhou.aliyuncs.com/web-config/docs-sdk/WbXwhniecMNLxKDj.webp" alt="dex1-1 gripper" style="width: 25%;">
  </a>
</p>

The dexterous hand is controlled via serial communication, and the manufacturer provides C and Python [SDKs](https://www.brainco-hz.com/docs/revolimb-hand/revo2/parameters.html).

In this repository, we convert serial messages into DDS messages so they can be used with [unitree_sdk2](https://github.com/unitreerobotics/unitree_sdk2) or [unitree_sdk2_python](https://github.com/unitreerobotics/unitree_sdk2_python).

- Each hand (left or right) is controlled by a USB-to-serial device, and each generates a pair of topics: `rt/brainco/(left or right)/(cmd or state)`.

- The position and speed of the fingers are normalized to the [0, 1] range.

- It is recommended to set the speed of all fingers to 1.0.

- The finger indices are mapped as follows: [Thumb, Thumb_aux, Index, Middle, Ring, Pinky].

> Here is a similar project [unitree-g1-brainco-hand](https://github.com/BrainCoTech/unitree-g1-brainco-hand) you can refer to, which is adapted by BrainCoTech.

# 1. 📦 Installation

```bash
# at user development computing unit PC2 (NVIDIA Jetson Orin NX board)
sudo apt install libspdlog-dev libfmt-dev
cd ~
git clone https://github.com/unitreerobotics/brainco_hand_service
cd brainco_hand_service
mkdir build && cd build
cmake ..
make -j6
```

# 2. 🚀 Launch

```bash
cd ~/brainco_hand_service/bin
# Run `sudo ././brainco_hand_server -h` for details. The output will be:
# Unitree Brainco Hand Service:
#  -h [ --help ]                  produce help message
#  -v [ --version ]               show version
#  -n [ --network_interface ] arg dds network interface

# start server
sudo ./brainco_hand_server --network eth0
# Simplified (defaults apply)
sudo ./brainco_hand_server

# at another terminal, run test examples
# Usage: ./test_brainco_hand_server [left|right]
# Default is 'left' if not specified.
# Normally, you should see the dexterous hand repeatedly perform the actions of making a fist and opening.

# test left side
cd ~/brainco_hand_service/bin
sudo ./test_brainco_hand_server
# or test right side
sudo ./test_brainco_hand_server right
```

# 3. 🚀🚀🚀 Automatic Startup Service

After completing the above setup and configuration, and successfully testing test_brainco_hand_server, you can configure the test_brainco_hand_server to start automatically on system boot by running the following script:

```bash
cd ~/brainco_hand_service
bash setup_autostart.sh
```

Follow the prompts in the script to complete your configuration.



# ❓ FAQ

1. Error when `make -j6`:

   ```bash
   unitree@ubuntu:~/brainco_hand_service/build$ make -j6
   Scanning dependencies of target brainco_hand_server
   Scanning dependencies of target test_brainco_hand_server
   [ 50%] Building CXX object CMakeFiles/test_brainco_hand_server.dir/test/test_brainco_hand_server.cpp.o
   [ 50%] Building CXX object CMakeFiles/brainco_hand_server.dir/main.cpp.o
   /home/unitree/brainco_hand_service/test/test_brainco_hand_server.cpp:1:10: fatal error: unitree/idl/go2/MotorCmds_.hpp: No such file or directory
       1 | #include <unitree/idl/go2/MotorCmds_.hpp>
         |          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   /home/unitree/brainco_hand_service/main.cpp:1:10: fatal error: unitree/idl/go2/MotorCmds_.hpp: No such file or directory
       1 | #include <unitree/idl/go2/MotorCmds_.hpp>
         |          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   compilation terminated.
   compilation terminated.
   ```

   This error mentions that the unitree_sdk2 headfile could not be found. First compile and install unitree_sdk2:

   ```bash
   cd ~
   git clone https://github.com/unitreerobotics/unitree_sdk2
   cd unitree_sdk2
   mkdir build & cd build
   cmake ..
   sudo make install
   ```

   

