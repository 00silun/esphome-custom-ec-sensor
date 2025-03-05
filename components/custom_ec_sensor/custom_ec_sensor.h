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
    // Set defaults.
    k_value_ = 1.0;
    temperature_ = 25.0;
    voltage_1413_ = 0;
    voltage_12_88_ = 0;

    // Load stored k_value if available (using preference index 0).
    esphome::ESPPreferenceObject k_pref = esphome::global_preferences->make_preference<float>(0);
    if (k_pref.load(&k_value_)) {
      ESP_LOGD("EC Sensor", "Loaded K-value: %.2f", k_value_);
    }
    // Load persistent calibration flag (using preference index 1).
    esphome::ESPPreferenceObject cal_flag_pref = esphome::global_preferences->make_preference<bool>(1);
    if (cal_flag_pref.load(&calibration_indicator_)) {
      ESP_LOGD("EC Sensor", "Loaded calibration flag: %s", calibration_indicator_ ? "true" : "false");
    } else {
      calibration_indicator_ = false;
    }
  }

  void update() override {
    if (!ads_sensor_->has_state()) {
      ESP_LOGW("EC Sensor", "ADS1115 has no valid reading yet.");
      return;
    }

    // Assume ads_sensor_->state returns voltage in volts.
    float voltage = ads_sensor_->state;

    // Retrieve water temperature; default to 25°C if invalid.
    float temp = water_temperature_sensor_->state;
    if (!std::isfinite(temp)) {
      ESP_LOGW("EC Sensor", "Water temperature reading invalid; defaulting to 25°C");
      temp = 25.0;
    }
    temperature_ = temp;

    // Check if calibration has been completed.
    if (voltage_1413_ == 0 || voltage_12_88_ == 0) {
      ESP_LOGW("EC Sensor", "Calibration not complete. Using default conversion.");
      // Default linear conversion:
      // Map 0 - 3.4V linearly to 0 - 15 mS/cm.
      float ec_value_mS = voltage * (15.0 / 3.4);
      // Temperature compensation.
      ec_value_mS = ec_value_mS / (1.0 + 0.0185 * (temperature_ - 25.0));
      ESP_LOGD("EC Sensor", "Default Conversion: Raw Voltage: %.2f V, Temp: %.2f°C, EC: %.2f mS/cm",
               voltage, temperature_, ec_value_mS);
      publish_state(ec_value_mS);
      return;
    }

    // If calibration is complete, use two-point linear interpolation.
    // Compute the slope (k_value_) as:
    //   k_value_ = (12880 - 1413) / (voltage_12_88_ - voltage_1413_)
    // Then:
    //   EC (µS/cm) = 1413 + (voltage - voltage_1413_) * k_value_
    float ec_value_uS = 1413.0 + (voltage - voltage_1413_) * k_value_;
    // Temperature compensation.
    ec_value_uS = ec_value_uS / (1.0 + 0.0185 * (temperature_ - 25.0));
    // Convert µS/cm to mS/cm.
    float ec_value_mS = ec_value_uS / 1000.0;

    ESP_LOGD("EC Sensor", "Calibrated Conversion: Raw Voltage: %.2f V, Temp: %.2f°C, EC: %.2f µS/cm, %.2f mS/cm",
             voltage, temperature_, ec_value_uS, ec_value_mS);
    publish_state(ec_value_mS);
  }

  // Calibration functions (voltage in volts).
  void calibrate_ec_1413(float voltage) {
    // Standard solution: 1413 µS/cm.
    voltage_1413_ = voltage;
    update_k_value();
    ESP_LOGD("EC Sensor", "Stored calibration voltage for 1413 µS/cm: %.3f V", voltage);
  }

  void calibrate_ec_12_88(float voltage) {
    // Standard solution: 12.88 mS/cm (12880 µS/cm).
    voltage_12_88_ = voltage;
    update_k_value();
    ESP_LOGD("EC Sensor", "Stored calibration voltage for 12.88 mS/cm: %.3f V", voltage);
  }

  // Returns true if both calibration points are set and calibration is complete.
  bool is_calibrated() const {
    return (voltage_1413_ != 0 && voltage_12_88_ != 0 && calibration_indicator_);
  }

  // Resets the calibration indicator (without erasing stored calibration voltages).
  void reset_calibration_indicator() {
    calibration_indicator_ = false;
    esphome::ESPPreferenceObject cal_flag_pref = esphome::global_preferences->make_preference<bool>(1);
    cal_flag_pref.save(&calibration_indicator_);
    ESP_LOGD("EC Sensor", "Calibration indicator reset. Sensor appears uncalibrated to HA.");
  }

 private:
  esphome::sensor::Sensor *ads_sensor_;
  esphome::sensor::Sensor *water_temperature_sensor_;
  float k_value_;         // Slope (µS/cm per volt) computed from calibration.
  float temperature_;
  float voltage_1413_;    // Voltage (in V) measured in the 1413 µS/cm standard.
  float voltage_12_88_;   // Voltage (in V) measured in the 12.88 mS/cm standard.
  bool calibration_indicator_;  // True if calibration is complete.

  // Recalculate k_value_ based on the two calibration points.
  void update_k_value() {
    if (voltage_1413_ == 0 || voltage_12_88_ == 0) {
      ESP_LOGW("EC Sensor", "Both calibration points are needed for accurate calibration.");
      return;
    }
    k_value_ = (12880.0 - 1413.0) / (voltage_12_88_ - voltage_1413_);
    esphome::ESPPreferenceObject k_pref = esphome::global_preferences->make_preference<float>(0);
    k_pref.save(&k_value_);
    calibration_indicator_ = true;
    // Save the calibration flag persistently.
    esphome::ESPPreferenceObject cal_flag_pref = esphome::global_preferences->make_preference<bool>(1);
    cal_flag_pref.save(&calibration_indicator_);
    ESP_LOGD("EC Sensor", "Calibration Completed: K-value = %.2f", k_value_);
  }
};

}  // namespace custom_ec_sensor
}  // namespace esphome
