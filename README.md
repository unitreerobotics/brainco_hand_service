# BrainCo Hand Service

The G1 can be equipped with BrainCo's [2nd generation Dexterous Hand](https://www.brainco.cn/#/product/revo2), which features 6 degrees of freedom.

The dexterous hand is controlled via serial communication, and the manufacturer provides C and Python [SDKs](https://www.brainco-hz.com/docs/revolimb-hand/revo2/parameters.html).

In this repository, we convert serial messages into DDS messages so they can be used with unitree_sdk2.

- Each hand (left or right) is controlled by a USB-to-serial device, and each generates a pair of topics: `rt/brainco/(left or right)/(cmd or state)`.

- The position and speed of the fingers are normalized to the [0, 1] range.

- It is recommended to set the speed of all fingers to 1.0.

- The finger indices are mapped as follows: [Thumb, Thumb_aux, Index, Middle, Ring, Pinky].

## Setup

```bash
sudo apt install libspdlog-dev libfmt-dev
cd ~
git clone https://github.com/unitreerobotics/brainco_hand_service
cd ~/brainco_hand_service
git submodule update --init --depth 1
# You can also manually download the latest version of https://github.com/BrainCoTech/stark-serialport-example/tree/revo2.
cd thirdparty/stark-serialport-example
./download-lib.sh

cd ~/brainco_hand_service
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j6
```

## Test

```bash
# Terminal 1. Run brainco hand service (The serial port name will be adjusted according to your hardware interface)
sudo ./brainco_hand --serial /dev/ttyUSB0
# Terminal 2. Run example
./example_brainco_hand left # or right
```
