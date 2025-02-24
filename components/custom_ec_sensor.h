#pragma once
#include "esphome.h"
#include "esphome/components/ads1115/ads1115.h"

namespace custom_ec_sensor {

class EcSensor : public esphome::PollingComponent, public esphome::sensor::Sensor {
 public:
  // Constructor accepts the ADS sensor and water temperature sensor pointers.
  EcSensor(esphome::sensor::Sensor *ads_sensor, esphome::sensor::Sensor *water_temperature_sensor)
      : esphome::PollingComponent(1000),
        ads_sensor_(ads_sensor),
        water_temperature_sensor_(water_temperature_sensor) {}

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

    float voltage = ads_sensor_->state * 1000.0;  // Convert to millivolts
    // Use the water_temperature sensor pointer directly
    temperature_ = water_temperature_sensor_->state;

    float raw_ec_value = 1000 * voltage / RES2 / ECREF * k_value_ * 10.0;
    float ec_value_µS = raw_ec_value / (1.0 + 0.0185 * (temperature_ - 25.0));  // µS/cm with temperature compensation

    // Convert µS/cm to mS/cm
    float ec_value_mS = ec_value_µS / 1000.0;

    ESP_LOGD("EC Sensor", "EC: %.2f µS/cm, %.2f mS/cm", ec_value_µS, ec_value_mS);
    publish_state(ec_value_mS);
  }

  void calibrate_ec_1413(float voltage) {
    voltage_1413_ = voltage;
    update_k_value();
    ESP_LOGD("EC Sensor", "Stored calibration voltage for 1413 µS/cm: %.3f V", voltage);
  }

  void calibrate_ec_12_88(float voltage) {
    voltage_12_88_ = voltage;
    update_k_value();
    ESP_LOGD("EC Sensor", "Stored calibration voltage for 12.88 mS/cm: %.3f V", voltage);
  }

 private:
  esphome::sensor::Sensor *ads_sensor_;
  esphome::sensor::Sensor *water_temperature_sensor_;
  float k_value_;
  float temperature_;
  float voltage_1413_;
  float voltage_12_88_;
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
    ESP_LOGD("EC Sensor", "Calibration Completed: K-value = %.2f", k_value_);
  }
};

}  // namespace custom_ec_sensor
