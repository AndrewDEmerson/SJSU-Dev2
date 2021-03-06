#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

#include "L1_Peripheral/i2c.hpp"

#include "L1_Peripheral/cortex/interrupt.hpp"
#include "L0_Platform/lpc40xx/LPC40xx.h"
#include "L1_Peripheral/lpc40xx/pin.hpp"
#include "L1_Peripheral/lpc40xx/system_controller.hpp"
#include "utility/build_info.hpp"
#include "utility/enum.hpp"
#include "utility/log.hpp"
#include "utility/status.hpp"
#include "utility/time.hpp"

namespace sjsu
{
namespace lpc40xx
{
/// Implementation of the I2C peripheral for the LPC40xx family of
/// microcontrollers.
class I2c final : public sjsu::I2c
{
 public:
  // Bringing in I2c Interface's Write and WriteThenRead methods that use
  // std::initializer_list.
  using sjsu::I2c::Read;
  using sjsu::I2c::Write;
  using sjsu::I2c::WriteThenRead;

  /// lpc40xx i2c peripheral control register flags
  enum Control : uint32_t
  {
    kAssertAcknowledge = 1 << 2,  // AA
    kInterrupt         = 1 << 3,  // SI
    kStop              = 1 << 4,  // STO
    kStart             = 1 << 5,  // STA
    kInterfaceEnable   = 1 << 6   // I2EN
  };

  /// lpc40xx I2C peripheral state numbers
  enum class MasterState : uint32_t
  {
    kBusError                          = 0x00,
    kStartCondition                    = 0x08,
    kRepeatedStart                     = 0x10,
    kSlaveAddressWriteSentReceivedAck  = 0x18,
    kSlaveAddressWriteSentReceivedNack = 0x20,
    kTransmittedDataReceivedAck        = 0x28,
    kTransmittedDataReceivedNack       = 0x30,
    kArbitrationLost                   = 0x38,
    kSlaveAddressReadSentReceivedAck   = 0x40,
    kSlaveAddressReadSentReceivedNack  = 0x48,
    kReceivedDataReceivedAck           = 0x50,
    kReceivedDataReceivedNack          = 0x58,
    kOwnAddressReceived                = 0xA0,
    kDoNothing                         = 0xF8
  };

  /// Bus_t holds all of the information for an I2C bus on the LPC40xx platform.
  struct Bus_t
  {
    /// Holds a pointer to the LPC_I2C peripheral registers
    LPC_I2C_TypeDef * registers;
    /// ResourceID of the I2C peripheral to power on at initialization.
    sjsu::SystemController::ResourceID id;
    /// IRQ number for this I2C port.
    sjsu::cortex::IRQn_Type irq_number;
    /// A reference to the transaction structure for this specific port. Each
    /// port gets its own transaction structure. Only 1, because a I2C bus can
    /// only have a single transaction on its bus at a time.
    Transaction_t & transaction;
    /// Refernce to I2C data pin.
    const sjsu::Pin & sda_pin;
    /// Refernce to I2C clock pin.
    const sjsu::Pin & scl_pin;
    /// Function code to set each pin to the appropriate I2C function.
    uint8_t pin_function;
  };

  /// Structure used as a namespace for predefined Bus definitions
  struct Bus  // NOLINT
  {
   private:
    inline static const sjsu::lpc40xx::Pin kI2c0SdaPin =
        sjsu::lpc40xx::Pin::CreatePin<0, 0>();
    inline static const sjsu::lpc40xx::Pin kI2c0SclPin =
        sjsu::lpc40xx::Pin::CreatePin<0, 1>();
    inline static const sjsu::lpc40xx::Pin kI2c1SdaPin =
        sjsu::lpc40xx::Pin::CreatePin<1, 30>();
    inline static const sjsu::lpc40xx::Pin kI2c1SclPin =
        sjsu::lpc40xx::Pin::CreatePin<1, 31>();
    inline static const sjsu::lpc40xx::Pin kI2c2SdaPin =
        sjsu::lpc40xx::Pin::CreatePin<0, 10>();
    inline static const sjsu::lpc40xx::Pin kI2c2SclPin =
        sjsu::lpc40xx::Pin::CreatePin<0, 11>();

    inline static Transaction_t transaction_i2c0;
    inline static Transaction_t transaction_i2c1;
    inline static Transaction_t transaction_i2c2;

   public:
    // UM10562: Chapter 7: LPC408x/407x I/O configuration page 133
    /// Definition for I2C bus 0 for LPC40xx.
    inline static const Bus_t kI2c0 = {
      .registers    = LPC_I2C0,
      .id           = sjsu::lpc40xx::SystemController::Peripherals::kI2c0,
      .irq_number   = I2C0_IRQn,
      .transaction  = transaction_i2c0,
      .sda_pin      = kI2c0SdaPin,
      .scl_pin      = kI2c0SclPin,
      .pin_function = 0b010,
    };

    /// Definition for I2C bus 1 for LPC40xx.
    inline static const Bus_t kI2c1 = {
      .registers    = LPC_I2C1,
      .id           = sjsu::lpc40xx::SystemController::Peripherals::kI2c1,
      .irq_number   = I2C1_IRQn,
      .transaction  = transaction_i2c1,
      .sda_pin      = kI2c1SdaPin,
      .scl_pin      = kI2c1SclPin,
      .pin_function = 0b011,
    };

    /// Definition for I2C bus 2 for LPC40xx.
    inline static const Bus_t kI2c2 = {
      .registers    = LPC_I2C2,
      .id           = sjsu::lpc40xx::SystemController::Peripherals::kI2c2,
      .irq_number   = I2C2_IRQn,
      .transaction  = transaction_i2c2,
      .sda_pin      = kI2c2SdaPin,
      .scl_pin      = kI2c2SclPin,
      .pin_function = 0b010,
    };
  };

  /// I2C interrupt handler
  ///
  /// @param i2c - this function cannot normally be used as an ISR, so it needs
  ///        help from a template function, or some other static function to
  ///        pass it the appropriate Bus_t object.
  static void I2cHandler(const Bus_t & i2c)
  {
    MasterState state   = MasterState(i2c.registers->STAT);
    uint32_t clear_mask = 0;
    uint32_t set_mask   = 0;
    switch (state)
    {
      case MasterState::kBusError:  // 0x00
      {
        i2c.transaction.status = Status::kBusError;
        set_mask               = Control::kAssertAcknowledge | Control::kStop;
        break;
      }
      case MasterState::kStartCondition:  // 0x08
      {
        i2c.registers->DAT = i2c.transaction.GetProperAddress();
        break;
      }
      case MasterState::kRepeatedStart:  // 0x10
      {
        i2c.transaction.operation = Operation::kRead;
        i2c.registers->DAT        = i2c.transaction.GetProperAddress();
        break;
      }
      case MasterState::kSlaveAddressWriteSentReceivedAck:  // 0x18
      {
        clear_mask = Control::kStart;
        if (i2c.transaction.out_length == 0)
        {
          i2c.transaction.busy   = false;
          i2c.transaction.status = Status::kSuccess;
          set_mask               = Control::kStop;
        }
        else
        {
          size_t position    = i2c.transaction.position++;
          i2c.registers->DAT = i2c.transaction.data_out[position];
        }
        break;
      }
      case MasterState::kSlaveAddressWriteSentReceivedNack:  // 0x20
      {
        clear_mask             = Control::kStart;
        i2c.transaction.busy   = false;
        i2c.transaction.status = Status::kDeviceNotFound;
        set_mask               = Control::kStop;
        break;
      }
      case MasterState::kTransmittedDataReceivedAck:  // 0x28
      {
        if (i2c.transaction.position >= i2c.transaction.out_length)
        {
          if (i2c.transaction.repeated)
          {
            // OR with 1 to set address as READ for the next transaction
            i2c.transaction.operation = Operation::kRead;
            i2c.transaction.position  = 0;
            set_mask                  = Control::kStart;
          }
          else
          {
            i2c.transaction.busy = false;
            set_mask             = Control::kStop;
          }
        }
        else
        {
          size_t position    = i2c.transaction.position++;
          i2c.registers->DAT = i2c.transaction.data_out[position];
        }
        break;
      }
      case MasterState::kTransmittedDataReceivedNack:  // 0x30
      {
        i2c.transaction.busy = false;
        set_mask             = Control::kStop;
        break;
      }
      case MasterState::kArbitrationLost:  // 0x38
      {
        set_mask = Control::kStart;
        break;
      }
      case MasterState::kSlaveAddressReadSentReceivedAck:  // 0x40
      {
        clear_mask = Control::kStart;
        if (i2c.transaction.in_length == 0)
        {
          set_mask = Control::kStop;
        }
        // If we only want 1 byte, make sure to nack that byte
        else if (i2c.transaction.in_length == 1)
        {
          clear_mask |= Control::kAssertAcknowledge;
        }
        // If we want more then 1 byte, make sure to ack the first byte
        else
        {
          set_mask = Control::kAssertAcknowledge;
        }
        break;
      }
      case MasterState::kSlaveAddressReadSentReceivedNack:  // 0x48
      {
        clear_mask             = Control::kStart;
        i2c.transaction.status = Status::kDeviceNotFound;
        i2c.transaction.busy   = false;
        set_mask               = Control::kStop;
        break;
      }
      case MasterState::kReceivedDataReceivedAck:  // 0x50
      {
        const size_t kBufferEnd = i2c.transaction.in_length;
        if (i2c.transaction.position < kBufferEnd)
        {
          const size_t kPosition = i2c.transaction.position;
          i2c.transaction.data_in[kPosition] =
              static_cast<uint8_t>(i2c.registers->DAT);
          i2c.transaction.position++;
        }
        // Check if the position has been pushed past the buffer length
        if (i2c.transaction.position + 1 >= kBufferEnd)
        {
          clear_mask           = Control::kAssertAcknowledge;
          i2c.transaction.busy = false;
        }
        else
        {
          set_mask = Control::kAssertAcknowledge;
        }
        break;
      }
      case MasterState::kReceivedDataReceivedNack:  // 0x58
      {
        i2c.transaction.busy = false;
        if (i2c.transaction.in_length != 0)
        {
          size_t position = i2c.transaction.position++;
          i2c.transaction.data_in[position] =
              static_cast<uint8_t>(i2c.registers->DAT);
        }
        set_mask = Control::kStop;
        break;
      }
      case MasterState::kDoNothing:  // 0xF8
      {
        break;
      }
      default:
      {
        clear_mask = Control::kStop;
        SJ2_ASSERT_FATAL(false, "Invalid I2C State Reached!!");
        break;
      }
    }
    // Clear I2C Interrupt flag
    clear_mask |= Control::kInterrupt;
    // Set register controls
    i2c.registers->CONSET = set_mask;
    i2c.registers->CONCLR = clear_mask;
  }
  /// Constructor for LPC40xx I2c peripheral
  ///
  /// @param bus - pass a reference to a constant lpc40xx::I2c::Bus_t
  ///        definition.
  /// @param clock_rate - clock rate to operate the i2c bus at.
  explicit constexpr I2c(const Bus_t & bus,
                         units::frequency::hertz_t clock_rate = 100'000_Hz)
      : i2c_(bus), clock_rate_(clock_rate)
  {
  }

  Returns<void> Initialize() const override
  {
    i2c_.sda_pin.SetPinFunction(i2c_.pin_function);
    i2c_.scl_pin.SetPinFunction(i2c_.pin_function);
    i2c_.sda_pin.SetAsOpenDrain();
    i2c_.scl_pin.SetAsOpenDrain();
    i2c_.sda_pin.SetFloating();
    i2c_.scl_pin.SetFloating();

    auto & system = sjsu::SystemController::GetPlatformController();
    system.PowerUpPeripheral(i2c_.id);
    auto peripheral_frequency = system.GetClockRate(i2c_.id);

    // Calculating and setting the I2C Clock rate
    // Weight the high side duty cycle more than the lower side by 30% in order
    // to give more time for the bus to charge up.
    float clock_frequency = (peripheral_frequency / clock_rate_) / 2.0f;
    float scll            = clock_frequency * 0.7f;
    float sclh            = clock_frequency * 1.3f;
    i2c_.registers->SCLL  = static_cast<uint32_t>(scll);
    i2c_.registers->SCLH  = static_cast<uint32_t>(sclh);

    i2c_.registers->CONCLR = Control::kAssertAcknowledge | Control::kStart |
                             Control::kStop | Control::kInterrupt;
    i2c_.registers->CONSET = Control::kInterfaceEnable;

    sjsu::InterruptController::GetPlatformController().Enable({
        .interrupt_request_number = i2c_.irq_number,
        .interrupt_handler        = [this]() { I2cHandler(i2c_); },
    });

    return {};
  }

  Returns<void> Transaction(Transaction_t transaction) const override
  {
    i2c_.transaction       = transaction;
    i2c_.registers->CONSET = Control::kStart;
    return BlockUntilFinished();
  }

  /// Special method that returns the current state of the transaction.
  const Transaction_t GetTransactionInfo()
  {
    return i2c_.transaction;
  }

  /// Special method to check if the bus is currently initialized.
  /// @returns true if this bus has been initialized.
  bool IsIntialized() const
  {
    return (i2c_.registers->CONSET & Control::kInterfaceEnable);
  }

 private:
  /// Since this I2C implementation utilizes interrupts, while the transaction
  /// is happening, on the bus, block the sequence of execution until the
  /// transaction has completed, OR the timeout has elapsed.
  Returns<void> BlockUntilFinished() const
  {
    // Skip waiting on the interrupt if running a host unit test
    if constexpr (build::kPlatform == build::Platform::host)
    {
      return {};
    }

    if (!IsIntialized())
    {
      return Error(Status::kNotReadyYet,
                   "Attempted to use I2C, but peripheral was not initialized!"
                   "Be sure to run the Initialize() method first");
    }

    auto wait_for_i2c_transaction = [this]() -> bool {
      return !i2c_.transaction.busy;
    };

    Status wait_status =
        Wait(i2c_.transaction.timeout, wait_for_i2c_transaction);

    if (i2c_.transaction.status == Status::kBusError)
    {
      return DefinedError(CommonErrors::kBusError);
    }
    else if (i2c_.transaction.status == Status::kDeviceNotFound)
    {
      return DefinedError(CommonErrors::kDeviceNotFound);
    }
    else if (wait_status == Status::kTimedOut)
    {
      // Abort I2C communication if this point is reached!
      i2c_.registers->CONSET = Control::kAssertAcknowledge | Control::kStop;

      if (i2c_.transaction.out_length == 0 || i2c_.transaction.in_length == 0)
      {
        return DefinedError(CommonErrors::kTimeout);
      }
    }

    // Ensure that start is cleared before leaving this function
    i2c_.registers->CONCLR = Control::kStart;
    return {};
  }

  const Bus_t & i2c_;
  units::frequency::hertz_t clock_rate_;
};
}  // namespace lpc40xx
}  // namespace sjsu
