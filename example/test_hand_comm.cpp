#include "stark-sdk.h"

void get_device_info(ModbusHandle *handle, uint8_t slave_id)
{
  auto info = modbus_get_device_info(handle, slave_id);
  if (info != NULL)
  {
    printf("Slave[%hhu] SKU Type: %hhu, Serial Number: %s, Firmware Version: %s\n", slave_id, (uint8_t)info->sku_type, info->serial_number, info->firmware_version);
    free_device_info(info);
  }
  else
  {
    printf("Error: Failed to get device info\n");
  }
}

int main(int argc, char** argv)
{
    auto port_name = "/dev/ttyUSB1"; // Mac USB HUB
    uint8_t slave_id = 0x7e;

    uint32_t baudrate = 460800;
    init_cfg(StarkFirmwareType::STARK_FIRMWARE_TYPE_V2_STANDARD, StarkProtocolType::STARK_PROTOCOL_TYPE_MODBUS, LogLevel::LOG_LEVEL_INFO);

    auto handle = modbus_open(port_name, baudrate, slave_id);
    get_device_info(handle, slave_id);

    modbus_close(handle);
    return 0;
}