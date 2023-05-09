#pragma once

#include "esphome/components/climate_ir/climate_ir.h"
#include <ctime>

namespace esphome {
namespace whirlpool_ac {

/// Simple enum to represent models.
enum Model {
  MODEL_DG11J1_3A = 0,  /// Temperature range is from 18 to 32
  MODEL_DG11J1_91 = 1,  /// Temperature range is from 16 to 30
};

// Temperature
const float WHIRLPOOL_DG11J1_3A_TEMP_MAX = 32.0;
const float WHIRLPOOL_DG11J1_3A_TEMP_MIN = 18.0;
const float WHIRLPOOL_DG11J1_91_TEMP_MAX = 30.0;
const float WHIRLPOOL_DG11J1_91_TEMP_MIN = 16.0;

class WhirlpoolClimateAC : public climate_ir::ClimateIR {
 public:
  WhirlpoolClimateAC()
      : climate_ir::ClimateIR(temperature_min_(), temperature_max_(), 1.0f, true, true,
                              {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
                               climate::CLIMATE_FAN_HIGH},
                              {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL}) {}

  void setup() override {
    climate_ir::ClimateIR::setup();

    this->powered_on_assumed = this->mode != climate::CLIMATE_MODE_OFF;
    // Or set to 0 at setup stage
    this->t_transmit = timestamp_now();
    //this->t_transmit = esphome::time::ESPTime::timestamp;
    //ESP_LOGD("Started setup class");
  }

  /// Override control to change settings of the climate device.
  void control(const climate::ClimateCall &call) override {
    send_swing_cmd_ = call.get_swing_mode().has_value();
    climate_ir::ClimateIR::control(call);
  }

  void set_model(Model model) { this->model_ = model; }

  // used to track when to send the power toggle command
  bool powered_on_assumed;

  // used to track pause between send and receive IR commands 
  time_t t_receive, t_transmit;
  
  //time_t timestamp_now() { return ::time(nullptr); }
  auto timestamp_now() {
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    time(&now);
    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGD(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
    return now;
  }
  
  // used for send OFF state to HA in case out of sync
  void send_off() {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->powered_on_assumed = false;
    this->target_temperature = 21;
    this->fan_mode = climate::CLIMATE_FAN_AUTO;
    this->swing_mode = climate::CLIMATE_SWING_OFF;
    // this->name_
    this->publish_state();
  }
  
  void set_ir_sensor(sensor::Sensor *ir_sensor) { this->ir_sensor_ = ir_sensor; }

 protected:
  /// Transmit via IR the state of this climate controller.
  void transmit_state() override;
  /// Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override;

  bool send_swing_cmd_{false};
  Model model_;
  sensor::Sensor *ir_sensor_{nullptr};

  float temperature_min_() {
    return (model_ == MODEL_DG11J1_3A) ? WHIRLPOOL_DG11J1_3A_TEMP_MIN : WHIRLPOOL_DG11J1_91_TEMP_MIN;
  }
  float temperature_max_() {
    return (model_ == MODEL_DG11J1_3A) ? WHIRLPOOL_DG11J1_3A_TEMP_MAX : WHIRLPOOL_DG11J1_91_TEMP_MAX;
  }
};

}  // namespace whirlpool_ac
}  // namespace esphome
