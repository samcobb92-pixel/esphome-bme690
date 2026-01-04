#include "bme690.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace bme690 {

static const char *TAG = "bme690";

// Static pointer for callbacks
static BME690Component *instance_ = nullptr;

void BME690Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up BME690...");
  
  instance_ = this;
  
  // Initialize the sensor structure
  this->gas_sensor_.intf = BME69X_I2C_INTF;
  this->gas_sensor_.read = i2c_read_;
  this->gas_sensor_.write = i2c_write_;
  this->gas_sensor_.delay_us = delay_us_;
  this->gas_sensor_.intf_ptr = this;
  this->gas_sensor_.amb_temp = 25; // Ambient temperature for heater compensation
  
  // Initialize the sensor
  int8_t rslt = bme69x_init(&this->gas_sensor_);
  if (rslt != BME69X_OK) {
    ESP_LOGE(TAG, "Failed to initialize BME690 sensor! Error: %d", rslt);
    this->mark_failed();
    return;
  }
  
  // Get current configuration
  rslt = bme69x_get_conf(&this->conf_, &this->gas_sensor_);
  if (rslt != BME69X_OK) {
    ESP_LOGE(TAG, "Failed to get sensor configuration! Error: %d", rslt);
    this->mark_failed();
    return;
  }
  
  // Configure sensor settings
  this->conf_.os_hum = this->humidity_oversampling_;
  this->conf_.os_pres = this->pressure_oversampling_;
  this->conf_.os_temp = this->temperature_oversampling_;
  this->conf_.filter = this->iir_filter_coefficient_;
  this->conf_.odr = BME69X_ODR_NONE;
  
  rslt = bme69x_set_conf(&this->conf_, &this->gas_sensor_);
  if (rslt != BME69X_OK) {
    ESP_LOGE(TAG, "Failed to set sensor configuration! Error: %d", rslt);
    this->mark_failed();
    return;
  }
  
  // Configure heater
  this->heatr_conf_.enable = BME69X_ENABLE;
  this->heatr_conf_.heatr_temp = this->heater_temperature_;
  this->heatr_conf_.heatr_dur = this->heater_duration_;
  
  rslt = bme69x_set_heatr_conf(BME69X_FORCED_MODE, &this->heatr_conf_, &this->gas_sensor_);
  if (rslt != BME69X_OK) {
    ESP_LOGE(TAG, "Failed to set heater configuration! Error: %d", rslt);
    this->mark_failed();
    return;
  }
  
  ESP_LOGCONFIG(TAG, "BME690 setup complete");
}

void BME690Component::dump_config() {
  ESP_LOGCONFIG(TAG, "BME690:");
  LOG_I2C_DEVICE(this);
  
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with BME690 failed!");
  }
  
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("  ", "Pressure", this->pressure_sensor_);
  LOG_SENSOR("  ", "Humidity", this->humidity_sensor_);
  LOG_SENSOR("  ", "Gas Resistance", this->gas_resistance_sensor_);
  
  ESP_LOGCONFIG(TAG, "  Heater Temperature: %d°C", this->heater_temperature_);
  ESP_LOGCONFIG(TAG, "  Heater Duration: %dms", this->heater_duration_);
  ESP_LOGCONFIG(TAG, "  IIR Filter: %d", this->iir_filter_coefficient_);
}

void BME690Component::update() {
  // Set sensor to forced mode to trigger measurement
  int8_t rslt = bme69x_set_op_mode(BME69X_FORCED_MODE, &this->gas_sensor_);
  if (rslt != BME69X_OK) {
    ESP_LOGW(TAG, "Failed to set sensor mode! Error: %d", rslt);
    this->status_set_warning();
    return;
  }
  
  // Calculate measurement duration
  uint32_t del_period = bme69x_get_meas_dur(BME69X_FORCED_MODE, &this->conf_, &this->gas_sensor_) + 
                        (this->heatr_conf_.heatr_dur * 1000);
  
  // Wait for measurement to complete
  this->gas_sensor_.delay_us(del_period, this->gas_sensor_.intf_ptr);
  
  // Read sensor data
  struct bme69x_data data;
  uint8_t n_fields;
  rslt = bme69x_get_data(BME69X_FORCED_MODE, &data, &n_fields, &this->gas_sensor_);
  
  if (rslt != BME69X_OK) {
    ESP_LOGW(TAG, "Failed to read sensor data! Error: %d", rslt);
    this->status_set_warning();
    return;
  }
  
  if (n_fields > 0) {
    // Publish temperature
    if (this->temperature_sensor_ != nullptr) {
      float temperature = data.temperature;
      this->temperature_sensor_->publish_state(temperature);
    }
    
    // Publish pressure
    if (this->pressure_sensor_ != nullptr) {
      float pressure = data.pressure / 100.0f; // Convert Pa to hPa
      this->pressure_sensor_->publish_state(pressure);
    }
    
    // Publish humidity
    if (this->humidity_sensor_ != nullptr) {
      float humidity = data.humidity;
      this->humidity_sensor_->publish_state(humidity);
    }
    
    // Publish gas resistance (only if valid)
    if (this->gas_resistance_sensor_ != nullptr && 
        (data.status & BME69X_GASM_VALID_MSK)) {
      float gas_resistance = data.gas_resistance;
      this->gas_resistance_sensor_->publish_state(gas_resistance);
    }
    
    this->status_clear_warning();
  } else {
    ESP_LOGW(TAG, "No new data available");
  }
}

// Static I2C read function
BME69X_INTF_RET_TYPE BME690Component::i2c_read_(uint8_t reg_addr, uint8_t *reg_data, 
                                                 uint32_t len, void *intf_ptr) {
  BME690Component *component = static_cast<BME690Component *>(intf_ptr);
  
  if (!component->read_register(reg_addr, reg_data, len)) {
    return BME69X_E_COM_FAIL;
  }
  
  return BME69X_OK;
}

// Static I2C write function
BME69X_INTF_RET_TYPE BME690Component::i2c_write_(uint8_t reg_addr, const uint8_t *reg_data, 
                                                  uint32_t len, void *intf_ptr) {
  BME690Component *component = static_cast<BME690Component *>(intf_ptr);
  
  if (!component->write_register(reg_addr, reg_data, len)) {
    return BME69X_E_COM_FAIL;
  }
  
  return BME69X_OK;
}

// Static delay function
void BME690Component::delay_us_(uint32_t period, void *intf_ptr) {
  delayMicroseconds(period);
}

}  // namespace bme690
}  // namespace esphome
