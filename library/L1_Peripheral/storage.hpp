#pragma once

#include <cstdint>
#include <cstddef>

#include "utility/units.hpp"
#include "utility/status.hpp"

namespace sjsu
{
/// Abstract interface for persistent memory storage systems.
/// @ingroup l1_peripheral
class Storage
{
 public:
  /// Defines the types of storage media.
  enum class Type
  {
    /// Simple storage implemented in RAM
    kRam,
    /// Raw NOR flash, without a sophisticated controller
    kNor,
    /// Raw NAND flash, without a sophisticated controller
    kNand,
    /// SD card
    kSD,
    /// eMMC managed NAND flash memory
    kEMMC,
    /// Electrically Erasable Programmable Read-Only Memory
    kEeprom,
    /// Non-volatile RAM
    kNvRam,
    /// Ferromagnetic RAM
    kFRam,
  };

  /// @return the type of memory this driver controls. Can be called without
  ///         calling Initialize() first.
  virtual Type GetMemoryType() = 0;

  /// Initialize all peripherals required to make communicate with the storage
  /// media possible. MUST be called before calling any method in this interface
  /// with the exception of `GetMemoryType()`
  virtual Returns<void> Initialize() = 0;

  /// Will prepare and configure storage media for communication.
  /// This method can only be called after `Initialize()` was called
  /// successfully without returning an `Error_t`.
  /// This method MUST be called in order for the following methods to operate:
  ///
  ///    bool IsReadOnly()
  ///    units::data::byte_t GetCapacity()
  ///    units::data::byte_t GetBlockSize()
  ///    Returns<void> Erase(uint32_t, size_t)
  ///    Returns<void> Write(uint32_t, const void *, size_t)
  ///    Returns<void> Read(uint32_t, void *, size_t)
  ///    Returns<void> Disable()
  ///
  /// Calling any of these methods before `Enable()` has been called
  /// successfully, will result in undefined behavior.
  virtual Returns<void> Enable() = 0;

  /// @return true if the storage is present. For cases where the memory cannot
  ///         be removed or is physically located within a device, this should
  ///         always return true.
  /// @return false if storage media is not present.
  virtual bool IsMediaPresent() = 0;

  /// @return true if device is not writable.
  virtual bool IsReadOnly() = 0;

  /// @return the maximum capacity of this storage media. This includes areas
  ///         that have already been written to. This does not include sections
  ///         of the memory that are not accessible. For example, if the first
  ///         2kB of the memory cannot be accessed via this driver, then it
  ///         should not be considered as apart of the capacity.
  virtual units::data::byte_t GetCapacity() = 0;

  /// @return the number of bytes per block.
  virtual units::data::byte_t GetBlockSize() = 0;

  /// Must be called before a `Write()` operation. Erases the contents of the
  /// storage media in the location specified, the number of bytes given. Some
  /// storage media do not require an erasure before a write to be performed. In
  /// those cases, the implementation should simply do nothing on erase.
  ///
  /// @param block_address - starting block to erase.
  /// @param blocks_count - the number of bytes to erase.
  /// @return Status of if the operation was successful, otherwise, returns an
  ///         appropriate status signal.
  virtual Returns<void> Erase(uint32_t block_address, size_t blocks_count) = 0;

  /// Write data to the storage media in the location block specified. If the
  /// block size for this media is not 1 byte, then single byte is not allowed
  /// and transactions must be performed at a block level.
  ///
  /// Be sure to call Erase() on the memory location you plan to write to before
  /// writing, otherwise, the data may be corrupted or unchanged.
  ///
  /// @param block_address - starting block to write to
  /// @param data - data to be stored in the location addressed above.
  /// @param size - the number of bytes to write into memory. Can be less than
  ///               the size of a block.
  /// @return Status of if the operation was successful, otherwise, returns an
  ///         appropriate status signal.
  virtual Returns<void> Write(uint32_t block_address,
                              const void * data,
                              size_t size) = 0;

  /// Read data from the storage media in the location specified.
  ///
  /// @param block_address - starting block to read from.
  /// @param data - pointer to where read data should be stored.
  /// @param size - the number of bytes to read from memory. Can be less than
  ///               the size of a block.
  /// @return Status of if the operation was successful, otherwise, return an
  ///         appropriate status signal.
  virtual Returns<void> Read(uint32_t block_address,
                             void * data,
                             size_t size) = 0;

  /// Should shutdown the device.
  virtual Returns<void> Disable() = 0;
};
}  // namespace sjsu
