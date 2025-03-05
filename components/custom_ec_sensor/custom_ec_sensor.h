#pragma once
#include "esphome.h"
#include "esphome/components/ads1115/ads1115.h"
#include <cmath>  // For std::isfinite

namespace esphome {
namespace custom_ec_sensor {

class EcSensor : public esphome::PollingComponent, public esphome::sensor::Sensor {
 public:
  // Constructor accepts pointers for the ADS sensor and water temperature sensor.
  EcSensor(esphome::sensor::Sensor *ads_sensor, esphome::sensor::Sensor *water_temperature_sensor)
      : esphome::PollingComponent(1000),
        ads_sensor_(ads_sensor),
        water_temperature_sensor_(water_temperature_sensor),
        calibration_indicator_(false) {}

  void setup() override {
    k_value_ = 1.0;         // Default K-value for calibration
    temperature_ = 25.0;    // Default temperature for compensation
    voltage_1413_ = 0;
    voltage_12_88_ = 0;

    esphome::ESPPreferenceObject k_pref = esphome::global_preferences->make_preference<float>(0);
    if (k_pref.load(&k_value_)) {
      ESP_LOGD("EC Sensor", "Loaded K-value: %.2f", k_value_);
    }
  }

  void update() override {
    if (!ads_sensor_->has_state()) {
      ESP_LOGW("EC Sensor", "ADS1115 has no valid reading yet.");
      return;
    }

    // Use the sensor reading in volts directly.
    float voltage = ads_sensor_->state;

    // Retrieve water temperature; default to 25°C if invalid.
    float temp = water_temperature_sensor_->state;
    if (!std::isfinite(temp)) {
      ESP_LOGW("EC Sensor", "Water temperature reading invalid; defaulting to 25°C");
      temp = 25.0;
    }
    temperature_ = temp;

    // Calculate raw EC value.
    // Original formula in mV was:
    //   raw_ec_value = 1000 * voltage(mV) / RES2 / ECREF * k_value_ * 10.0;
    // Since voltage (in V) * 1000 equals voltage in mV,
    //   raw_ec_value = (voltage * 1000 * 1000 * 10.0 * k_value_) / (RES2 * ECREF)
    //                = (10000000.0 * voltage * k_value_) / (RES2 * ECREF)
    float raw_ec_value = (10000000.0 * voltage * k_value_) / (RES2 * ECREF);
    float ec_value_µS = raw_ec_value / (1.0 + 0.0185 * (temperature_ - 25.0));  // µS/cm with temperature compensation

    // Convert µS/cm to mS/cm.
    float ec_value_mS = ec_value_µS / 1000.0;

    ESP_LOGD("EC Sensor", "Raw Voltage: %.2f V, Temp: %.2f°C, EC: %.2f µS/cm, %.2f mS/cm",
             voltage, temperature_, ec_value_µS, ec_value_mS);
    publish_state(ec_value_mS);
  }

  void calibrate_ec_1413(float voltage) {
    // Expect voltage in volts.
    voltage_1413_ = voltage;
    update_k_value();
    ESP_LOGD("EC Sensor", "Stored calibration voltage for 1413 µS/cm: %.3f V", voltage);
  }

  void calibrate_ec_12_88(float voltage) {
    // Expect voltage in volts.
    voltage_12_88_ = voltage;
    update_k_value();
    ESP_LOGD("EC Sensor", "Stored calibration voltage for 12.88 mS/cm: %.3f V", voltage);
  }

  // Returns true if both calibration points are set and the indicator flag is true.
  bool is_calibrated() const {
    return (voltage_1413_ != 0 && voltage_12_88_ != 0 && calibration_indicator_);
  }

  // Resets only the calibration indicator so that is_calibrated() returns false.
  // The stored calibration data (k_value_, voltage_1413_, voltage_12_88_) remain intact.
  void reset_calibration_indicator() {
    calibration_indicator_ = false;
    ESP_LOGD("EC Sensor", "Calibration indicator reset. Sensor appears uncalibrated to HA.");
  }

 private:
  esphome::sensor::Sensor *ads_sensor_;
  esphome::sensor::Sensor *water_temperature_sensor_;
  float k_value_;
  float temperature_;
  float voltage_1413_;
  float voltage_12_88_;
  bool calibration_indicator_;  // Flag to indicate if calibration is valid.
  static constexpr float RES2 = 7500.0 / 0.66;
  static constexpr float ECREF = 20.0;

  void update_k_value() {
    if (voltage_1413_ == 0 || voltage_12_88_ == 0) {
      ESP_LOGW("EC Sensor", "Both calibration points needed for accurate calibration.");
      return;
    }
    k_value_ = (12880.0 - 1413.0) / (voltage_12_88_ - voltage_1413_);
    esphome::ESPPreferenceObject k_pref = esphome::global_preferences->make_preference<float>(0);
    k_pref.save(&k_value_);
    calibration_indicator_ = true;  // Mark as calibrated once both points are set.
    ESP_LOGD("EC Sensor", "Calibration Completed: K-value = %.2f", k_value_);
  }
};

}  // namespace custom_ec_sensor
}  // namespace esphome
