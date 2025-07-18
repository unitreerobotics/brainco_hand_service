# BrainCo Hand Service

The G1 can be equipped with BrainCo's [2nd generation Dexterous Hand](https://www.brainco-hz.com/docs/revolimb-hand/product/overview.html), which features 6 degrees of freedom.

The dexterous hand is controlled via serial communication, and the manufacturer provides C and Python [SDKs](https://www.brainco-hz.com/docs/revolimb-hand/sdk/c_v2.html).

In this repository, we convert serial messages into DDS messages so they can be used with unitree_sdk2.

- Each hand (left or right) is controlled by a USB-to-serial device, and each generates a pair of topics: `rt/brainco/(left or right)/(cmd or state)`.

- The position and speed of the fingers are normalized to the [0, 1] range.

- It is recommended to set the speed of all fingers to 1.0.

- The finger indices are mapped as follows: [Thumb, Thumb_aux, Index, Middle, Ring, Pinky].

## Setup

```bash
sudo apt install libspdlog-dev libfmt-dev
# you may need to dowload the newest version of `stark-serialport-example` manually.
# git clone https://github.com/BrainCoTech/stark-serialport-example
cd thirdparty/stark-serialport-example
./download-lib.sh

cd path/to/brainco_hand_service
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

## Test

```bash
# Terminal 1. Run brainco hand service
sudo ./brainco_hand --id 126 --serial /dev/ttyUSB0 # 126: left hand, 127: right hand
# Terminal 2. Run example
./example_brainco_hand left # or right
```
