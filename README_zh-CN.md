<div align="center">
  <h1 align="center">
    <a href="https://www.unitree.com" target="_blank">Brainco Hand Service</a>
  </h1>
  <a href="https://www.unitree.com/" target="_blank">
    <img src="https://www.unitree.com/images/0079f8938336436e955ea3a98c4e1e59.svg" alt="Unitree LOGO" width="15%">
  </a>
  <p align="center">
    <a href="README.md"> English </a> | <a>中文</a> </a>
  </p>
</div>


# 0. 📖 介绍

G1 可以搭载[强脑科技](https://www.brainco.cn)的 [第二代仿生灵巧手 Revo2](https://www.brainco.cn/#/product/revo2)，它具有 6 个自由度。

<p align="center">
  <a href="https://brainco-common-public.oss-cn-hangzhou.aliyuncs.com/web-config/docs-sdk/WbXwhniecMNLxKDj.webp">
    <img src="https://brainco-common-public.oss-cn-hangzhou.aliyuncs.com/web-config/docs-sdk/WbXwhniecMNLxKDj.webp" alt="dex1-1 gripper" style="width: 25%;">
  </a>
</p>


灵巧手通过串口进行控制，厂商提供了 C 和 Python 的 [SDK](https://www.brainco-hz.com/docs/revolimb-hand/revo2/parameters.html)。

在本仓库中，我们将串口消息转换成 DDS 消息，以便可以与 [unitree_sdk2](https://github.com/unitreerobotics/unitree_sdk2) 或 [unitree_sdk2_python](https://github.com/unitreerobotics/unitree_sdk2_python) 配合使用。

* 每只手（左或右）通过一个 USB-转-串口设备进行控制，并各自生成一对主题：`rt/brainco/(left or right)/(cmd or state)`。

* 手指的位置和速度都被归一化到 [0, 1] 的范围。

* 推荐将所有手指速度都设置为 1.0。

* 手指索引映射如下：[拇指、拇指副指、食指、中指、无名指、小指]。

> 你还可以参考一个类似项目 [unitree-g1-brainco-hand](https://github.com/BrainCoTech/unitree-g1-brainco-hand)，该项目由 BrainCoTech 适配。

# 1. 📦 安装

```bash
# 在用户开发计算单元 PC2（NVIDIA Jetson Orin NX 板）上
sudo apt install libspdlog-dev libfmt-dev
cd ~
git clone https://github.com/unitreerobotics/brainco_hand_service
cd brainco_hand_service
mkdir build && cd build
cmake ..
make -j6
```

# 2. 🚀 启动

```bash
cd ~/brainco_hand_service/bin
# 运行 `sudo ./brainco_hand_server -h` 获取更多信息。输出如下：
# Unitree Brainco Hand Service:
#  -h [ --help ]                  显示帮助信息
#  -v [ --version ]               显示版本
#  -n [ --network_interface ] arg 指定 DDS 网络接口

# 启动服务
sudo ./brainco_hand_server --network eth0
# 简化（使用默认配置）
sudo ./brainco_hand_server

# 另一个终端，运行测试示例
# 用法: ./test_brainco_hand_server [left|right]
# 若未指定，默认为 left。
# 正常情况下，你会看到灵巧手反复做握拳和张开动作。

# 测试左手
cd ~/brainco_hand_service/bin
sudo ./test_brainco_hand_server
# 或测试右手
sudo ./test_brainco_hand_server right
```

# 3. 🚀🚀🚀 开机自启服务

完成上述安装和配置，并成功运行 test_brainco_hand_server 后，你可以通过以下脚本将 test_brainco_hand_server 配置为系统开机自动启动：

```bash
cd ~/brainco_hand_service
bash setup_autostart.sh
```

根据脚本提示完成配置即可。



# ❓ 常见问题

1. `make -j6` 出错：

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

   该错误说明 unitree_sdk2 头文件未找到。先编译并安装 unitree_sdk2：

   ```bash
   cd ~
   git clone https://github.com/unitreerobotics/unitree_sdk2
   cd unitree_sdk2
   mkdir build & cd build
   cmake ..
   sudo make install
   ```
