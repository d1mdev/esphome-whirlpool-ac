#pragma once

#include "esphome/components/climate_ir/climate_ir.h"
#include "esphome/components/switch/switch.h"

namespace esphome {
namespace whirlpool_ac {

/// Simple enum to represent models.
enum Model {
  MODEL_DG11J1_3A = 0,  /// Temperature range is from 18 to 32
  MODEL_DG11J1_91 = 1,  /// Temperature range is from 16 to 30
};

/// Simple enum to represent internal states of iFeel mode.
enum Mode {
  OFF = 1,
  OFF_ON = 2,
  UPDATE = 3,
  ON_OFF = 4,
  ON = 5,
  REMOTE_CONTROLLED = 6,
};

// Temperature
const float WHIRLPOOL_DG11J1_3A_TEMP_MAX = 32.0;
const float WHIRLPOOL_DG11J1_3A_TEMP_MIN = 18.0;
const float WHIRLPOOL_DG11J1_91_TEMP_MAX = 30.0;
const float WHIRLPOOL_DG11J1_91_TEMP_MIN = 16.0;

class WhirlpoolAC : public climate_ir::ClimateIR {
 public:
  WhirlpoolAC()
      : climate_ir::ClimateIR(temperature_min_(), temperature_max_(), 1.0f, false, true,
                              {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
                               climate::CLIMATE_FAN_HIGH},
                              {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL}) {}

  void setup() override; /*{
//    climate_ir::ClimateIR::setup();
    if (this->sensor_) {
      this->sensor_->add_on_state_callback([this](float state) {
        this->current_temperature = state;
        // current temperature changed, publish state
        this->publish_state();
        if (this->ifeel_state_ && (millis() - this->ifeel_start_time_ > 300000) && (abs(this->current_temperature - this->target_temperature) > 1)) {
          ESP_LOGD(TAG, "Sending iFeel update. ");
        }
      });
      this->current_temperature = this->sensor_->state;
    } else
      this->current_temperature = NAN;
    // restore set points
    auto restore = this->restore_state_();
    if (restore.has_value()) {
      restore->apply(this);
    } else {
      // restore from defaults
      this->mode = climate::CLIMATE_MODE_OFF;
      // initialize target temperature to some value so that it's not NAN
      this->target_temperature =
        roundf(clamp(this->current_temperature, this->minimum_temperature_, this->maximum_temperature_));
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
      this->swing_mode = climate::CLIMATE_SWING_OFF;
      this->preset = climate::CLIMATE_PRESET_NONE;
    }
    // Never send nan to HA
    if (std::isnan(this->target_temperature))
      this->target_temperature = 24;

      this->powered_on_assumed = this->mode != climate::CLIMATE_MODE_OFF;
  }
*/
  /// Override control to change settings of the climate device.
  void control(const climate::ClimateCall &call) override {
    send_swing_cmd_ = call.get_swing_mode().has_value();
    climate_ir::ClimateIR::control(call);
  }

  void set_model(Model model) { this->model_ = model; }
  void set_ifeel_mode(Mode mode) { this->ifeel_mode_ = mode; }

  // used to track when to send the power toggle command
  bool powered_on_assumed;
    
  void set_ir_transmitter_switch(switch_::Switch *ir_transmitter_switch);
  void set_ifeel_switch(switch_::Switch *ifeel_switch);

 protected:
  /// Transmit via IR the state of this climate controller.
  void transmit_state() override;
  /// Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override;
  /// Stores the state of switch to prevent duplicate packets
  bool ir_transmitter_state_ = false;
  bool ifeel_state_ = false;
  bool ifeel_switching_ = false;
  /// Set the time of the last transmission.
  int32_t last_transmit_time_{};
  
  int32_t ifeel_start_time_{};

  bool send_swing_cmd_{false};
  Model model_;
  Mode ifeel_mode_;
  
  switch_::Switch *ir_transmitter_switch_ = nullptr;  // Switch to toggle IR mute on/off
  switch_::Switch *ifeel_switch_ = nullptr;  // Switch to toggle iFeel mode on/off

  void update_ir_transmitter(bool ir_transmitter);
  void update_ifeel(bool ifeel);
  void on_ir_transmitter_change(bool ir_transmitter);
  void on_ifeel_change(bool ifeel);
  void on_current_temperature_update(float state);

  float temperature_min_() {
    return (model_ == MODEL_DG11J1_3A) ? WHIRLPOOL_DG11J1_3A_TEMP_MIN : WHIRLPOOL_DG11J1_91_TEMP_MIN;
  }
  float temperature_max_() {
    return (model_ == MODEL_DG11J1_3A) ? WHIRLPOOL_DG11J1_3A_TEMP_MAX : WHIRLPOOL_DG11J1_91_TEMP_MAX;
  }
};

}  // namespace whirlpool_ac
}  // namespace esphome
