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
                              {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL},
                              {climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_ACTIVITY}) {} //NONE = 0, ACTIVITY = 7

  void setup() override {
    climate_ir::ClimateIR::setup();

    this->powered_on_assumed = this->mode != climate::CLIMATE_MODE_OFF;
  }

  /// Override control to change settings of the climate device.
  void control(const climate::ClimateCall &call) override {
    send_swing_cmd_ = call.get_swing_mode().has_value();
    climate_ir::ClimateIR::control(call);
  }

  void set_model(Model model) { this->model_ = model; }

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
  /// Set the time of the last transmission.
  int32_t last_transmit_time_{};

  bool send_swing_cmd_{false};
  Model model_;
  
  switch_::Switch *ir_transmitter_switch_ = nullptr;  // Switch to toggle IR mute on/off
  switch_::Switch *ifeel_switch_ = nullptr;  // Switch to toggle iFeel mode on/off

  void update_ir_transmitter(bool ir_transmitter);
  void update_ifeel(bool ifeel);
  void on_ir_transmitter_change(bool ir_transmitter) = 0;
  void on_ifeel_change(bool ifeel) = 0;

  float temperature_min_() {
    return (model_ == MODEL_DG11J1_3A) ? WHIRLPOOL_DG11J1_3A_TEMP_MIN : WHIRLPOOL_DG11J1_91_TEMP_MIN;
  }
  float temperature_max_() {
    return (model_ == MODEL_DG11J1_3A) ? WHIRLPOOL_DG11J1_3A_TEMP_MAX : WHIRLPOOL_DG11J1_91_TEMP_MAX;
  }
};

}  // namespace whirlpool_ac
}  // namespace esphome
