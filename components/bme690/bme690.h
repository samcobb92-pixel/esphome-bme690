#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

// Include BME69x API files (works for BME680, BME688, and BME690)
extern "C" {
#include "bme69x.h"
}

namespace esphome {
namespace bme690 {

class BME690Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_temperature_sensor(sensor::Sensor *temperature_sensor) { 
    temperature_sensor_ = temperature_sensor; 
  }
  void set_pressure_sensor(sensor::Sensor *pressure_sensor) { 
    pressure_sensor_ = pressure_sensor; 
  }
  void set_humidity_sensor(sensor::Sensor *humidity_sensor) { 
    humidity_sensor_ = humidity_sensor; 
  }
  void set_gas_resistance_sensor(sensor::Sensor *gas_resistance_sensor) { 
    gas_resistance_sensor_ = gas_resistance_sensor; 
  }

  void set_heater_temperature(uint16_t temperature) { 
    heater_temperature_ = temperature; 
  }
  void set_heater_duration(uint16_t duration) { 
    heater_duration_ = duration; 
  }
  void set_temperature_oversampling(uint8_t oversampling) { 
    temperature_oversampling_ = oversampling; 
  }
  void set_pressure_oversampling(uint8_t oversampling) { 
    pressure_oversampling_ = oversampling; 
  }
  void set_humidity_oversampling(uint8_t oversampling) { 
    humidity_oversampling_ = oversampling; 
  }
  void set_iir_filter(uint8_t coefficient) { 
    iir_filter_coefficient_ = coefficient; 
  }

 protected:
  struct bme69x_dev gas_sensor_;
  struct bme69x_conf conf_;
  struct bme69x_heatr_conf heatr_conf_;
  
  uint16_t heater_temperature_{320};
  uint16_t heater_duration_{150};
  uint8_t temperature_oversampling_{BME69X_OS_8X};
  uint8_t pressure_oversampling_{BME69X_OS_4X};
  uint8_t humidity_oversampling_{BME69X_OS_2X};
  uint8_t iir_filter_coefficient_{3};
  
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *pressure_sensor_{nullptr};
  sensor::Sensor *humidity_sensor_{nullptr};
  sensor::Sensor *gas_resistance_sensor_{nullptr};

  // I2C read/write helper functions
  static BME69X_INTF_RET_TYPE i2c_read_(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
  static BME69X_INTF_RET_TYPE i2c_write_(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
  static void delay_us_(uint32_t period, void *intf_ptr);
};

}  // namespace bme690
}  // namespace esphome
