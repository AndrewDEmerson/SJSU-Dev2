// @ingroup SJSU-Dev2
// @defgroup TFMini driver
// @brief This driver provides the software to use the TFMini, a time of flight
// sensor
// @{
#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <numeric>

#include "L1_Peripheral/uart.hpp"
#include "L2_HAL/sensors/distance/distance_sensor.hpp"
#include "utility/status.hpp"
#include "utility/log.hpp"

namespace sjsu
{
/// Distance sensor driver for the TFMini LiDAR module
/// Find link here: https://www.sparkfun.com/products/14588
class TFMini final : public DistanceSensor
{
 public:
  /// Default timeout for reading responses from the TFMini LiDar module.
  /// The device's test latency is 100Hz, which is 10ms per sample, thus a
  /// timeout time of 3x that should give enough time to acquire the sample and
  /// respond back via UART.
  static constexpr std::chrono::nanoseconds kTimeout = 3 * 10ms;

  /// Response packet frame header used for detecting the start of a proper
  /// response packet.
  static constexpr uint8_t kFrameHeader = 0x59;

  /// Used to set the device to long range mode (currently not utilized)
  static constexpr uint8_t kLongDistMode = 0x07;

  /// Sensor signal strength lower bounds
  static constexpr uint8_t kStrengthLowerLimitCap = 80;

  /// Sensor signal strength upper bounds
  static constexpr uint32_t kStrengthUpperBound = 3000;

  /// Device response packet length
  static constexpr uint8_t kDeviceDataLength = 9;

  /// Device command packet length
  static constexpr uint8_t kCommandLength = 8;

  /// Command packet to enable configuration of device
  static constexpr uint8_t kConfigCommand[kCommandLength] = {
    0x42, 0x57, 0x02, 0x00, 0x00, 0x00, 0x01, 0x02,
  };

  /// Command packet to disable configuration of device
  static constexpr uint8_t kExitConfigCommand[kCommandLength] = {
    0x42, 0x57, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02
  };

  /// Command packet to set device units to millimeters.
  static constexpr uint8_t kSetDistUnitMM[kCommandLength] = {
    0x42, 0x57, 0x02, 0x00, 0x00, 0x00, 0x00, 0x1A,
  };

  /// Command packet to allow distance capture on kPromptMeasurementCommand
  /// command.
  static constexpr uint8_t kSetExternalTriggerMode[kCommandLength] = {
    0x42, 0x57, 0x02, 0x00, 0x00, 0x00, 0x00, 0x40
  };

  /// Command packet to request distance measurement data along with signal
  /// strength data.
  static constexpr uint8_t kPromptMeasurementCommand[kCommandLength] = {
    0x42, 0x57, 0x02, 0x00, 0x00, 0x00, 0x00, 0x41
  };

  /// @param uart - UART peripheral connected to the RX and TX of the TFMini
  ///               module.
  explicit constexpr TFMini(Uart & uart) : uart_pin_(uart) {}

  /// Initialize and enable hardware. This must be called before any other
  /// method in this interface is called.
  /// Sets the baud rate as well as configures the TFMini to have an external
  /// trigger and sets the distance units to mm
  ///
  /// @returns Status::kBusError if initialization fails
  /// @returns Status::kSuccess if initialization succeeds
  Status Initialize() override
  {
    constexpr uint32_t kBaudRate = 115200;
    Status init_success          = uart_pin_.Initialize(kBaudRate);

    if (IsOk(init_success))
    {
      if (!SendCommandAndCheckEcho(kConfigCommand) ||
          !SendCommandAndCheckEcho(kSetExternalTriggerMode) ||
          !SendCommandAndCheckEcho(kSetDistUnitMM) ||
          !SendCommandAndCheckEcho(kExitConfigCommand))
      {
        return sjsu::Status::kBusError;
      }
    }
    return Status::kSuccess;
  }

  /// Obtain the distance to the object directly in front of the sensor
  ///
  /// @param distance - the distance an object is from the TFMini in mm.
  ///        If the signal strength is lower than the min threshold, or if the
  ///        device is not recognized, the distance will be unreliable and set
  ///        to std::numeric_limits<uint32_t>::max(). If the actual distance is
  ///        greater than 12000mm, the distance value caps at 12000
  ///
  /// @returns Status::kDeviceNotFound if device is not recognized
  /// @returns Status::kBusError if data read from device is inconsistent
  /// @returns Status::kSuccess if device is successfully read from
  Status GetDistance(units::length::millimeter_t * distance) override
  {
    std::array<uint8_t, kDeviceDataLength> device_data = { 0 };

    uart_pin_.Write(kPromptMeasurementCommand, kCommandLength);
    uart_pin_.Read(device_data.data(), device_data.size(), kTimeout);

    if (device_data[0] != kFrameHeader || device_data[1] != kFrameHeader)
    {
      *distance = std::numeric_limits<units::length::millimeter_t>::max();
      return sjsu::Status::kDeviceNotFound;
    }

    uint8_t checksum =
        std::accumulate(device_data.begin(), &device_data.end()[-2], 0);

    if (checksum != device_data[8])
    {
      *distance = std::numeric_limits<units::length::millimeter_t>::max();
      return sjsu::Status::kBusError;
    }

    uint32_t dist = device_data[2];
    dist |= device_data[3] << 8;
    *distance = units::length::millimeter_t(dist);
    return sjsu::Status::kSuccess;
  }

  /// Obtain the strength of the light pulse the sensor emits.
  ///
  /// @param strength - the strength of the light pulse, calculated by
  ///        strength_val / 3000. Recommended that readings are reliable if in
  ///        range .7%-67%, lower limit is adjustable -> SetMinSignalThreshhold
  ///
  /// @returns Status::kDeviceNotFound if device is not recognized
  /// @returns Status::kBusError if data read from device is inconsistent
  /// @returns Status::kSuccess if device is successfully read from
  Status GetSignalStrengthPercent(float * strength) override
  {
    std::array<uint8_t, kDeviceDataLength> device_data = { 0 };

    uart_pin_.Write(kPromptMeasurementCommand, kCommandLength);
    uart_pin_.Read(device_data.data(), device_data.size(), kTimeout);

    if (device_data[0] != kFrameHeader || device_data[1] != kFrameHeader)
    {
      *strength = -1;
      return sjsu::Status::kDeviceNotFound;
    }

    uint8_t checksum =
        std::accumulate(device_data.begin(), &device_data.end()[-2], 0);

    if (checksum != device_data[8])
    {
      *strength = -1;
      return sjsu::Status::kBusError;
    }

    uint32_t strength_bytes = device_data[4];
    strength_bytes |= device_data[5] << 8;
    *strength = (strength_bytes / kStrengthUpperBound);

    return sjsu::Status::kSuccess;
  }

  /// Update the signal strength's lower limit valid range.
  ///
  /// @param lower_threshold - the value to set the lower threshold as.
  ///        default value is 20, caps at 80. Decrease the value to increase the
  ///        measurement range, increase value to improve reliability.
  ///
  /// @returns Status::kBusError if updating the TFMini fails
  /// @returns Status::kSuccess if device is successfully updated
  Status SetMinSignalThreshhold(uint8_t lower_threshold = 20)
  {
    constexpr uint8_t kUpdateMinThresholdCommand[kCommandLength] = {
      0x42, 0x57, 0x02, 0x00, 0xEE, 0x00, 0x00, 0x20
    };
    constexpr uint8_t kThresholdByte = 4;
    std::array<uint8_t, kCommandLength> updated_min_threshold_command;

    uint8_t low_limit = lower_threshold;

    if (low_limit > kStrengthLowerLimitCap)
    {
      low_limit = kStrengthLowerLimitCap;
    }
    for (int i = 0; i < kCommandLength; i++)
    {
      updated_min_threshold_command[i] = kUpdateMinThresholdCommand[i];
    }

    updated_min_threshold_command[kThresholdByte] = low_limit;

    if (!SendCommandAndCheckEcho(kConfigCommand) ||
        !SendCommandAndCheckEcho(updated_min_threshold_command.data()) ||
        !SendCommandAndCheckEcho(kExitConfigCommand))
    {
      return sjsu::Status::kBusError;
    }

    min_threshold_ = lower_threshold;

    return sjsu::Status::kSuccess;
  }

 private:
  bool SendCommandAndCheckEcho(const uint8_t * command) const
  {
    constexpr uint8_t kEchoSuccess[kCommandLength] = {
      0x42, 0x57, 0x02, 0x01, 0x00, 0x00, 0x01, 0x02,
    };
    constexpr uint8_t kConfigExit[kCommandLength] = {
      0x42, 0x57, 0x02, 0x01, 0x00, 0x00, 0x00, 0x02,
    };

    uint8_t echo[kCommandLength];

    uart_pin_.Write(command, kCommandLength);
    uart_pin_.Read(echo, sizeof(echo), kTimeout);
    for (int i = 0; i < kCommandLength; i++)
    {
      if ((kEchoSuccess[i] != echo[i]) && (kConfigExit[i] != echo[i]))
      {
        return false;
      }
    }
    return true;
  }

  const Uart & uart_pin_;
  uint8_t min_threshold_ = 20;
};
}  // namespace sjsu
