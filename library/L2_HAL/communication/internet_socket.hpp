#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "utility/status.hpp"
#include "utility/units.hpp"

namespace sjsu
{
/// @defgroup communication Communication Interfaces

/// A network socket is an internal endpoint for sending or receiving data from
/// within computer network. It is an endpoint in networking software.
/// Modeled after Berkley Sockets (POSIX sockets)
/// @ingroup communication
class InternetSocket
{
 public:
  /// Which Internet Protocol to use for communicating with the remote host
  /// server.
  enum class Protocol
  {
    kTCP,
    kUDP,
  };

  /// Establishes a direct communication link to a specific remote host
  /// identified by its address, port, and the communication protocol.
  ///
  /// @param protocol - Using TCP or UDP
  /// @param address - URL of the device you want to connect to.
  /// @param port - The port you want to use to connect to the host.
  /// @param timeout - Amount of time before this function should gives up.
  /// @return Status::kTimeout if this function runs out of time based on
  ///         timeout.
  /// @return Status::kSuccess if this function is successful.
  /// @return Can potentially return any other status.
  virtual Status Connect(Protocol protocol,
                         std::string_view address,
                         uint16_t port,
                         std::chrono::nanoseconds timeout) = 0;

  /// Send data to the connected host. Must have used Connect() before using
  /// this.
  ///
  /// @param data - data to write to socket
  /// @param size - the number of bytes to write.
  /// @param timeout - Amount of time before this function should gives up.
  /// @return Status::kTimeout if this function runs out of time based on
  ///         timeout.
  /// @return Status::kSuccess if this function is successful.
  /// @return Can potentially return any other status.
  virtual Status Write(const void * data,
                       size_t size,
                       std::chrono::nanoseconds timeout) = 0;

  /// Read data received from the connected host. Must have used Connect()
  /// before using this.
  ///
  /// @param buffer - location to read information from
  /// @param size - the number of bytes to write.
  /// @param timeout - Amount of time before this function should gives up.
  /// @return Status::kTimeout if this function runs out of time based on
  ///         timeout.
  /// @return Status::kSuccess if this function is successful.
  /// @return Can potentially return any other status.
  virtual size_t Read(void * buffer,
                      size_t size,
                      std::chrono::nanoseconds timeout) = 0;

  /// Closes the connection established by the Connect() method.
  ///
  /// @return Status::kTimeout if this function runs out of time based on
  ///         timeout.
  /// @return Status::kSuccess if this function is successful.
  /// @return Status::kNotReadyYet if this was called before Connect was used.
  /// @return Can potentially return any other status.
  virtual Status Close() = 0;
};

/// An interface for devices that can communicate wirelessly via the Wifi
/// protocol. This interface is used for connecting a device to a Wifi hotspot
/// (client).
/// @ingroup communication
class WiFi
{
 public:
  /// Contains network connection information such as IP address, netmask,
  /// gateway, and mac address.
  struct NetworkConnection_t
  {
    /// If `status` is Status::kSuccess then the fields below are correct.
    /// Otherwise, this status holds what went wrong.
    Status status = Status::kNotImplemented;
    /// IPv4 IP address
    std::array<uint8_t, 4> ip;
    /// IPv4 IP address netmask
    std::array<uint8_t, 4> netmask;
    /// IPv4 gateway IP address
    std::array<uint8_t, 4> gateway;
    /// Device mac address
    std::array<uint8_t, 6> mac;
  };

  /// The type of password security used for the access point.
  /// NOTE: Used within the ESP8266 code but not in the interface
  enum class AccessPointSecurity
  {
    kOpen       = 0,
    kWep        = 1,
    kWpaPsk     = 2,
    kWpa2Psk    = 3,
    kWpaWpa2Psk = 4,
  };

  /// The type of mode to put the device into.
  /// NOTE: Used within the ESP8266 code but not in the interface
  enum class WifiMode
  {
    kClient = 1,
    kAccessPoint,
    kBoth,
  };

  /// Initialize the WiFi hardware and necessary peripherals needed to
  /// communicate with it.
  /// @return Any status.
  virtual Status Initialize() = 0;

  /// @return true - if this Wifi instance is connected to an access point
  /// @return false - it this Wifi instance is not connected to an access point
  virtual bool IsConnected() = 0;

  /// Requests to connects to an access point.
  ///
  /// @param ssid - SSID of the access point you would like to connect to.
  /// @param password - the password to the access point.
  /// @param timeout - Amount of time before this function should gives up.
  /// @return Status - Status::kSuccess if it is successful
  /// @return Status - Status::kTimeout if it is not successful
  virtual Status ConnectToAccessPoint(std::string_view ssid,
                                      std::string_view password,
                                      std::chrono::nanoseconds timeout) = 0;

  /// Disconnect from the access point.
  /// @return Status
  virtual Status DisconnectFromAccessPoint() = 0;

  /// @return NetworkConnection_t - Get connection information such as IP
  ///         address.
  virtual NetworkConnection_t GetNetworkConnectionInfo() = 0;

  /// @return InternetSocket& that can be used to connect and communicate over
  ///         TCP or UDP.
  virtual InternetSocket & GetInternetSocket() = 0;
};

}  // namespace sjsu
