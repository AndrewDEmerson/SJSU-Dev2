#include "L1_Peripheral/lpc17xx/adc.hpp"
#include "L1_Peripheral/lpc17xx/system_controller.hpp"
#include "L2_HAL/sensors/environment/light/temt6000x01.hpp"
#include "utility/log.hpp"

int main()
{
  sjsu::LogInfo("Starting LPC176x/5x Light Sensor Example...");

  // A 10kOhm pull-down resistor is used on the SJOne board.
  constexpr units::impedance::ohm_t kPullDownResistance = 10'000_Ohm;

  // The SJOne board uses ADC Channel 0.2, P0.25 for the light sensor
  sjsu::lpc17xx::Adc adc2(sjsu::lpc17xx::AdcChannel::kChannel2);

  sjsu::Temt6000x01 light_sensor(adc2, kPullDownResistance);
  SJ2_ASSERT_FATAL(light_sensor.Initialize() == sjsu::Status::kSuccess,
                   "Failed to initialized light sensor!");

  while (true)
  {
    sjsu::LogInfo(
        "Lux: %.4f, Brightness Percentage: %.2f%%",
        light_sensor.GetIlluminance().to<double>(),
        static_cast<double>(light_sensor.GetPercentageBrightness() * 100));
    sjsu::Delay(1s);
  }

  return 0;
}
