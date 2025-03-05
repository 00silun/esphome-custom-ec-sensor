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
    // Set defaults: k_value will be recalculated during calibration.
    k_value_ = 1.0;
    temperature_ = 25.0;
    voltage_1413_ = 0;
    voltage_12_88_ = 0;

    // Load stored k_value if available.
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

    // Assume ads_sensor_->state returns voltage in volts.
    float voltage = ads_sensor_->state;

    // Retrieve water temperature; default to 25°C if invalid.
    float temp = water_temperature_sensor_->state;
    if (!std::isfinite(temp)) {
      ESP_LOGW("EC Sensor", "Water temperature reading invalid; defaulting to 25°C");
      temp = 25.0;
    }
    temperature_ = temp;

    // Make sure calibration has been performed.
    if (voltage_1413_ == 0 || voltage_12_88_ == 0) {
      ESP_LOGW("EC Sensor", "Calibration not complete. Using default conversion.");
      // Optionally, you could publish a default value or skip measurement.
      return;
    }

    // Two-point linear interpolation:
    // EC (µS/cm) = 1413 + (voltage - voltage_1413_) * k_value_
    float ec_value_uS = 1413.0 + (voltage - voltage_1413_) * k_value_;
    // Apply temperature compensation (assuming 0.0185 per °C deviation from 25°C).
    ec_value_uS = ec_value_uS / (1.0 + 0.0185 * (temperature_ - 25.0));
    // Convert µS/cm to mS/cm.
    float ec_value_mS = ec_value_uS / 1000.0;

    ESP_LOGD("EC Sensor", "Raw Voltage: %.2f V, Temp: %.2f°C, EC: %.2f µS/cm, %.2f mS/cm",
             voltage, temperature_, ec_value_uS, ec_value_mS);
    publish_state(ec_value_mS);
  }

  // Calibration: Call these functions with a measured voltage (in volts)
  // when the probe is immersed in the standard solutions.
  void calibrate_ec_1413(float voltage) {
    // This calibration point corresponds to 1413 µS/cm.
    voltage_1413_ = voltage;
    update_k_value();
    ESP_LOGD("EC Sensor", "Stored calibration voltage for 1413 µS/cm: %.3f V", voltage);
  }

  void calibrate_ec_12_88(float voltage) {
    // This calibration point corresponds to 12.88 mS/cm (12880 µS/cm).
    voltage_12_88_ = voltage;
    update_k_value();
    ESP_LOGD("EC Sensor", "Stored calibration voltage for 12.88 mS/cm: %.3f V", voltage);
  }

  // Returns true if both calibration points are set and calibration has been performed.
  bool is_calibrated() const {
    return (voltage_1413_ != 0 && voltage_12_88_ != 0 && calibration_indicator_);
  }

  // Resets the calibration indicator (without erasing stored voltages).
  void reset_calibration_indicator() {
    calibration_indicator_ = false;
    ESP_LOGD("EC Sensor", "Calibration indicator reset. Sensor appears uncalibrated to HA.");
  }

 private:
  esphome::sensor::Sensor *ads_sensor_;
  esphome::sensor::Sensor *water_temperature_sensor_;
  float k_value_;         // Slope in µS/cm per volt, computed from calibration.
  float temperature_;
  float voltage_1413_;    // Measured voltage (in V) in the 1413 µS/cm standard solution.
  float voltage_12_88_;   // Measured voltage (in V) in the 12.88 mS/cm standard solution.
  bool calibration_indicator_;  // True if calibration is complete.

  // Recalculate k_value based on the two calibration points.
  void update_k_value() {
    if (voltage_1413_ == 0 || voltage_12_88_ == 0) {
      ESP_LOGW("EC Sensor", "Both calibration points needed for accurate calibration.");
      return;
    }
    // k_value_ is the slope (µS/cm per volt).
    k_value_ = (12880.0 - 1413.0) / (voltage_12_88_ - voltage_1413_);
    // Save k_value_ persistently.
    esphome::ESPPreferenceObject k_pref = esphome::global_preferences->make_preference<float>(0);
    k_pref.save(&k_value_);
    calibration_indicator_ = true;  // Mark sensor as calibrated.
    ESP_LOGD("EC Sensor", "Calibration Completed: K-value = %.2f", k_value_);
  }
};

}  // namespace custom_ec_sensor
}  // namespace esphome
