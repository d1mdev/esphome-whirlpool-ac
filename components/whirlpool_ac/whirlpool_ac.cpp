#include "whirlpool_ac.h"
#include "esphome/core/log.h"

namespace esphome {
namespace whirlpool_ac {

static const char *const TAG = "whirlpool_ac.climate";

const uint16_t WHIRLPOOL_HEADER_MARK = 9000;
const uint16_t WHIRLPOOL_HEADER_SPACE = 4494;
const uint16_t WHIRLPOOL_BIT_MARK = 572;
const uint16_t WHIRLPOOL_ONE_SPACE = 1659;
const uint16_t WHIRLPOOL_ZERO_SPACE = 553;
const uint32_t WHIRLPOOL_GAP = 7960;

const uint32_t WHIRLPOOL_CARRIER_FREQUENCY = 38000;

const uint8_t WHIRLPOOL_STATE_LENGTH = 21;

const uint8_t WHIRLPOOL_HEAT = 0;
const uint8_t WHIRLPOOL_DRY = 3;
const uint8_t WHIRLPOOL_COOL = 2;
const uint8_t WHIRLPOOL_FAN = 4;
const uint8_t WHIRLPOOL_AUTO = 1;

const uint8_t WHIRLPOOL_FAN_AUTO = 0;
const uint8_t WHIRLPOOL_FAN_HIGH = 1;
const uint8_t WHIRLPOOL_FAN_MED = 2;
const uint8_t WHIRLPOOL_FAN_LOW = 3;

const uint8_t WHIRLPOOL_SWING_MASK = 128;

const uint8_t WHIRLPOOL_POWER = 0x04;

void WhirlpoolAC::setup () {
//    climate_ir::ClimateIR::setup();
    if (this->sensor_) {
      this->sensor_->add_on_state_callback([this](float state) {
        this->current_temperature = state;
        this->on_current_temperature_update(state);
        // current temperature changed, publish state
        this->publish_state();
/*         ESP_LOGD(TAG, "ifeel_start_time_ (mins) - %d, delta - %d", (this->ifeel_start_time_ / 60000), (millis() - this->ifeel_start_time_) / 60000);
        if (this->ifeel_state_ && (millis() - this->ifeel_start_time_ > 120000) && (abs(this->current_temperature - this->target_temperature) > 1) && this->powered_on_assumed) {
          ESP_LOGD(TAG, "Sending iFeel update. ");
          this->ifeel_start_time_ = millis();
          this->transmit_state();
        } */
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

void WhirlpoolAC::transmit_state() {
  this->last_transmit_time_ = millis();  // setting the time of the last transmission.
  uint8_t remote_state[WHIRLPOOL_STATE_LENGTH] = {0};
  remote_state[0] = 0x83;
  remote_state[1] = 0x06;
  remote_state[6] = 0x80;
  // MODEL DG11J191
  remote_state[18] = 0x08;

  auto powered_on = this->mode != climate::CLIMATE_MODE_OFF;
  if (powered_on != this->powered_on_assumed) {
    // Set power toggle command
    remote_state[2] = 4;
    remote_state[15] = 1;
    this->powered_on_assumed = powered_on;
  }
  switch (this->mode) {
    case climate::CLIMATE_MODE_HEAT_COOL:
      // set fan auto
      // set temp auto temp
      // set sleep false
      remote_state[3] = WHIRLPOOL_AUTO;
      remote_state[15] = 0x17;
      break;
    case climate::CLIMATE_MODE_HEAT:
      remote_state[3] = WHIRLPOOL_HEAT;
      remote_state[15] = 6;
      break;
    case climate::CLIMATE_MODE_COOL:
      remote_state[3] = WHIRLPOOL_COOL;
      remote_state[15] = 6;
      break;
    case climate::CLIMATE_MODE_DRY:
      remote_state[3] = WHIRLPOOL_DRY;
      remote_state[15] = 6;
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      remote_state[3] = WHIRLPOOL_FAN;
      remote_state[15] = 6;
      break;
    case climate::CLIMATE_MODE_OFF:
//      this->update_ifeel(false);
//      break;
    default:
      break;
  }

  // Temperature
  auto temp = (uint8_t) roundf(clamp(this->target_temperature, this->temperature_min_(), this->temperature_max_()));
  remote_state[3] |= (uint8_t) (temp - this->temperature_min_()) << 4;

  // Fan speed
  switch (this->fan_mode.value()) {
    case climate::CLIMATE_FAN_HIGH:
      remote_state[2] |= WHIRLPOOL_FAN_HIGH;
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      remote_state[2] |= WHIRLPOOL_FAN_MED;
      break;
    case climate::CLIMATE_FAN_LOW:
      remote_state[2] |= WHIRLPOOL_FAN_LOW;
      break;
    default:
      break;
  }

  // Swing
  ESP_LOGV(TAG, "send swing %s", this->send_swing_cmd_ ? "true" : "false");
  if (this->send_swing_cmd_) {
    if (this->swing_mode == climate::CLIMATE_SWING_VERTICAL || this->swing_mode == climate::CLIMATE_SWING_OFF) {
      remote_state[2] |= 128;
      remote_state[8] |= 64;
    }
  }
  
  // iFeel
  // Check value of ifeel_mode_
  switch (this->ifeel_mode_) {
    case OFF_ON:
      ESP_LOGD(TAG, "Turning iFeel ON");
      if (remote_state[15] != 1) {
        remote_state[15] = 0x0D;
      }
      remote_state[11] = 0x80;
      if (!std::isnan(this->current_temperature)) {
        ESP_LOGD(TAG, "Sending current_temperature to AC. ");
        remote_state[12] = roundf(this->current_temperature);
      } else {
        ESP_LOGD(TAG, "Sending target_temperature to AC. ");
        remote_state[12] = this->target_temperature;
      }
      set_ifeel_mode(ON);
      break;
    case ON:
      ESP_LOGD(TAG, "Ifeel mode active. ");
      break;
    case UPDATE:
      ESP_LOGD(TAG, "Updating iFeel. ");
      remote_state[11] = 0x80;
      remote_state[15] = 0;
      if (!std::isnan(this->current_temperature)) {
        ESP_LOGD(TAG, "Sending current_temperature to AC. ");
        remote_state[12] = roundf(this->current_temperature);
      } else {
        ESP_LOGD(TAG, "Sending target_temperature to AC. ");
        remote_state[12] = this->target_temperature;
      }
      set_ifeel_mode(ON);
      break;
    case ON_OFF:
      ESP_LOGD(TAG, "Turning iFeel OFF. ");
      remote_state[15] = 0x0D;
      remote_state[12] = 0;
      set_ifeel_mode(OFF);
      break;
    case OFF:
      ESP_LOGD(TAG, "Ifeel mode is OFF. ");
      break;
    case REMOTE_CONTROLLED:
      ESP_LOGD(TAG, "Controlled from remote. Nothing to do. ");
      break;
    default:
      ESP_LOGD(TAG, "No iFeel mode set. ");
      break;
  }
/*   if (this->ifeel_state_) {
    ESP_LOGD(TAG, "iFeel switch is ON. ");
    remote_state[11] = 0x80;
    if (!std::isnan(this->current_temperature)) {
      ESP_LOGD(TAG, "Sending current_temperature to AC. ");
      remote_state[12] = roundf(this->current_temperature);
    } else {
      ESP_LOGD(TAG, "Sending target_temperature to AC. ");
      remote_state[12] = this->target_temperature;
    }
  }
  if (this->ifeel_switching_) {
    remote_state[15] = 0x0D;
    this->ifeel_switching_ = false;
  } */

/*   switch (this->preset.value()) {
    case climate::CLIMATE_PRESET_NONE:
      ESP_LOGD(TAG, "Asking for preset: NONE");
      break;
    case climate::CLIMATE_PRESET_ACTIVITY:
      // Только в режимах COOL, HEAT и HEAT_COOL
      // this->current_temperature != this->target_temperature - разница в температурах больше 2 град
      if ((this->mode == climate::CLIMATE_MODE_HEAT_COOL || this->mode == climate::CLIMATE_MODE_COOL || this->mode == climate::CLIMATE_MODE_HEAT) && !std::isnan(this->current_temperature)) {
        ESP_LOGD(TAG, "Asking for preset: ACTIVITY ");
        remote_state[15] = 0x0D;
        remote_state[12] = roundf(this->current_temperature);
      } else {
        ESP_LOGD(TAG, "Preset disabled ");
        this->preset = climate::CLIMATE_PRESET_NONE;
      }
      break;
    default:
      break;
  } */

  // Checksum
  for (uint8_t i = 2; i < 13; i++)
    remote_state[13] ^= remote_state[i];
  for (uint8_t i = 14; i < 20; i++)
    remote_state[20] ^= remote_state[i];

  ESP_LOGD(TAG,
           "Sending: %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X "
           "%02X %02X   %02X",
           remote_state[0], remote_state[1], remote_state[2], remote_state[3], remote_state[4], remote_state[5],
           remote_state[6], remote_state[7], remote_state[8], remote_state[9], remote_state[10], remote_state[11],
           remote_state[12], remote_state[13], remote_state[14], remote_state[15], remote_state[16], remote_state[17],
           remote_state[18], remote_state[19], remote_state[20]);

  // Send code
  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();

  data->set_carrier_frequency(WHIRLPOOL_CARRIER_FREQUENCY);

  // Header
  data->mark(WHIRLPOOL_HEADER_MARK);
  data->space(WHIRLPOOL_HEADER_SPACE);
  // Data
  auto bytes_sent = 0;
  for (uint8_t i : remote_state) {
    for (uint8_t j = 0; j < 8; j++) {
      data->mark(WHIRLPOOL_BIT_MARK);
      bool bit = i & (1 << j);
      data->space(bit ? WHIRLPOOL_ONE_SPACE : WHIRLPOOL_ZERO_SPACE);
    }
    bytes_sent++;
    if (bytes_sent == 6 || bytes_sent == 14) {
      // Divider
      data->mark(WHIRLPOOL_BIT_MARK);
      data->space(WHIRLPOOL_GAP);
    }
  }
  // Footer
  data->mark(WHIRLPOOL_BIT_MARK);

  if (this->ir_transmitter_switch_ != nullptr) {
    ESP_LOGD(TAG, "Detected transmitter MUTE switch. ");
  }
  ESP_LOGD(TAG, "TRANSMITTER IS %s. ", this->ir_transmitter_state_ ? "ON" : "OFF");
  ESP_LOGD(TAG, "ifeel_mode_ is: %d. ", this->ifeel_mode_);
  if (this->ir_transmitter_state_) {
    transmit.perform();
  }
}

bool WhirlpoolAC::on_receive(remote_base::RemoteReceiveData data) {
  // Check if the esp isn't currently transmitting
  if (millis() - this->last_transmit_time_ < 500) {
    ESP_LOGV(TAG, "Blocked receive because of current transmission");
    return false;
  }
  
  // Validate header
  if (!data.expect_item(WHIRLPOOL_HEADER_MARK, WHIRLPOOL_HEADER_SPACE)) {
    ESP_LOGV(TAG, "Header fail");
    return false;
  }
  
  uint8_t remote_state[WHIRLPOOL_STATE_LENGTH] = {0};
  // Read all bytes.
  for (int i = 0; i < WHIRLPOOL_STATE_LENGTH; i++) {
    // Read bit
    if (i == 6 || i == 14) {
      if (!data.expect_item(WHIRLPOOL_BIT_MARK, WHIRLPOOL_GAP))
        return false;
    }
    for (int j = 0; j < 8; j++) {
      if (data.expect_item(WHIRLPOOL_BIT_MARK, WHIRLPOOL_ONE_SPACE)) {
        remote_state[i] |= 1 << j;

      } else if (!data.expect_item(WHIRLPOOL_BIT_MARK, WHIRLPOOL_ZERO_SPACE)) {
        ESP_LOGV(TAG, "Byte %d bit %d fail", i, j);
        return false;
      }
    }

    ESP_LOGVV(TAG, "Byte %d %02X", i, remote_state[i]);
  }
  // Validate footer
  if (!data.expect_mark(WHIRLPOOL_BIT_MARK)) {
    ESP_LOGV(TAG, "Footer fail");
    return false;
  }

  uint8_t checksum13 = 0;
  uint8_t checksum20 = 0;
  // Calculate  checksum and compare with signal value.
  for (uint8_t i = 2; i < 13; i++)
    checksum13 ^= remote_state[i];
  for (uint8_t i = 14; i < 20; i++)
    checksum20 ^= remote_state[i];

  if (checksum13 != remote_state[13] || checksum20 != remote_state[20]) {
    ESP_LOGVV(TAG, "Checksum fail");
    return false;
  }

  ESP_LOGD(
      TAG,
      "Received: %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X %02X %02X   %02X %02X "
      "%02X %02X   %02X",
      remote_state[0], remote_state[1], remote_state[2], remote_state[3], remote_state[4], remote_state[5],
      remote_state[6], remote_state[7], remote_state[8], remote_state[9], remote_state[10], remote_state[11],
      remote_state[12], remote_state[13], remote_state[14], remote_state[15], remote_state[16], remote_state[17],
      remote_state[18], remote_state[19], remote_state[20]);

  // verify header remote code
  if (remote_state[0] != 0x83 || remote_state[1] != 0x06)
    return false;

  // powr on/off button
  ESP_LOGV(TAG, "Power: %02X", (remote_state[2] & WHIRLPOOL_POWER));

  if ((remote_state[2] & WHIRLPOOL_POWER) == WHIRLPOOL_POWER) {
    auto powered_on = this->mode != climate::CLIMATE_MODE_OFF;

    if (powered_on) {
      this->mode = climate::CLIMATE_MODE_OFF;
      this->powered_on_assumed = false;
    } else {
      this->powered_on_assumed = true;
    }
  }

  // Set received mode
  if (powered_on_assumed) {
    auto mode = remote_state[3] & 0x7;
    ESP_LOGV(TAG, "Mode: %02X", mode);
    switch (mode) {
      case WHIRLPOOL_HEAT:
        this->mode = climate::CLIMATE_MODE_HEAT;
        break;
      case WHIRLPOOL_COOL:
        this->mode = climate::CLIMATE_MODE_COOL;
        break;
      case WHIRLPOOL_DRY:
        this->mode = climate::CLIMATE_MODE_DRY;
        break;
      case WHIRLPOOL_FAN:
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
      case WHIRLPOOL_AUTO:
        this->mode = climate::CLIMATE_MODE_HEAT_COOL;
        break;
    }
  }

  // Set received temp
  int temp = remote_state[3] & 0xF0;
  ESP_LOGVV(TAG, "Temperature Raw: %02X", temp);
  temp = (uint8_t) temp >> 4;
  temp += static_cast<int>(this->temperature_min_());
  ESP_LOGVV(TAG, "Temperature Climate: %u", temp);
  this->target_temperature = temp;

  // Set received fan speed
  auto fan = remote_state[2] & 0x03;
  ESP_LOGVV(TAG, "Fan: %02X", fan);
  switch (fan) {
    case WHIRLPOOL_FAN_HIGH:
      this->fan_mode = climate::CLIMATE_FAN_HIGH;
      break;
    case WHIRLPOOL_FAN_MED:
      this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      break;
    case WHIRLPOOL_FAN_LOW:
      this->fan_mode = climate::CLIMATE_FAN_LOW;
      break;
    case WHIRLPOOL_FAN_AUTO:
    default:
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
      break;
  }

  // Set received swing status
  if ((remote_state[2] & WHIRLPOOL_SWING_MASK) == WHIRLPOOL_SWING_MASK && remote_state[8] == 0x40) {
    ESP_LOGVV(TAG, "Swing toggle pressed ");
    if (this->swing_mode == climate::CLIMATE_SWING_OFF) {
      this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
    } else {
      this->swing_mode = climate::CLIMATE_SWING_OFF;
    }
  }

  // Set received iFeel status
  if (remote_state[15] == 0x0D) {
    ESP_LOGD(TAG, "iFeel toggle pressed on remote. ");
    if (remote_state[11] == 0x80) {
      ESP_LOGD(TAG, "iFeel - Turned ON from remote. ");
      if (!std::isnan(this->current_temperature)) {
        int c_temp = remote_state[12];
        this->current_temperature = c_temp;
      }
      set_ifeel_mode(REMOTE_CONTROLLED);
      update_ifeel(true);
    } else {
      ESP_LOGD(TAG, "iFeel - Turned OFF from remote. ");
      set_ifeel_mode(OFF);
      update_ifeel(false);
    }
  }
  if (remote_state[15] == 0x00 && remote_state[11] == 0x80) {
    int c_temp = remote_state[12];
    ESP_LOGD(TAG, "Received iFeel temp update from remote. Temp is %d:", c_temp);
    this->current_temperature = c_temp;
    set_ifeel_mode(ON);
    update_ifeel(true);
  }
  this->publish_state();
  return true;
}

void WhirlpoolAC::update_ir_transmitter(bool ir_transmitter) {
  if (this->ir_transmitter_switch_ != nullptr) {
    this->ir_transmitter_state_ = ir_transmitter;
    this->ir_transmitter_switch_->publish_state(this->ir_transmitter_state_);
  }
}

void WhirlpoolAC::update_ifeel(bool ifeel) {
  if (this->ifeel_switch_ != nullptr) {
    ESP_LOGD(TAG, "update_ifeel. ");
    this->ifeel_state_ = ifeel;
    this->ifeel_switch_->publish_state(this->ifeel_state_);
  }
}

void WhirlpoolAC::set_ir_transmitter_switch(switch_::Switch *ir_transmitter_switch) {
  this->ir_transmitter_switch_ = ir_transmitter_switch;
  this->ir_transmitter_switch_->add_on_state_callback([this](bool state) {
    if (state == this->ir_transmitter_state_)
      return;
    this->on_ir_transmitter_change(state);
  });
}

void WhirlpoolAC::set_ifeel_switch(switch_::Switch *ifeel_switch) {
  this->ifeel_switch_ = ifeel_switch;
  this->ifeel_switch_->add_on_state_callback([this](bool state) {
    if (state == this->ifeel_state_)
      return;
    ESP_LOGD(TAG, "set_ifeel_switch. ");
    this->on_ifeel_change(state);
  });
}

void WhirlpoolAC::on_ir_transmitter_change(bool state) {
  this->ir_transmitter_state_ = state;
  if (state) {
    ESP_LOGV(TAG, "Turning on IR transmitter. ");
  } else {
    ESP_LOGV(TAG, "Turning off IR transmitter. ");
  }
}

void WhirlpoolAC::on_ifeel_change(bool state) {
  ESP_LOGD(TAG, "on_ifeel_change. ");
  this->ifeel_state_ = state;
  if (state) {
    ESP_LOGV(TAG, "Turning on iFeel. ");
    set_ifeel_mode(OFF_ON);
  } else {
    ESP_LOGV(TAG, "Turning off iFeel. ");
    set_ifeel_mode(ON_OFF);
  }
  if (this->powered_on_assumed) {
    this->ifeel_switching_ = true;
    this->ifeel_start_time_ = millis();
    this->transmit_state();
  } else {
    ESP_LOGD(TAG, "No switch activity while AC is OFF. ");
  }
/*   this->ifeel_state_ = state;
  this->ifeel_switching_ = true;
  if (state) {
    ESP_LOGV(TAG, "Turning on iFeel. ");
    set_ifeel_mode(OFF_ON);
  } else {
    ESP_LOGV(TAG, "Turning off iFeel. ");
    set_ifeel_mode(ON_OFF);
  } */
}

void WhirlpoolAC::on_current_temperature_update(float state) {
  ESP_LOGD(TAG, "--------------------------------------------");
  ESP_LOGD(TAG, "Get updates from sensor. State is: %.1f", state);
  ESP_LOGD(TAG, "iFeel state is: %s", this->ifeel_state_ ? "ON" : "OFF");
  ESP_LOGD(TAG, "ifeel_start_time_ (mins) - %d, current - %d", (this->ifeel_start_time_ / 60000), (millis() / 60000));
  ESP_LOGD(TAG, "--------------------------------------------");
  if (this->ifeel_state_ && (millis() - this->ifeel_start_time_ > 120000) && this->powered_on_assumed) {
    ESP_LOGD(TAG, "Sending iFeel update. ");
    this->ifeel_start_time_ = millis();
    set_ifeel_mode(UPDATE);
    this->transmit_state();
  }
}
}  // namespace whirlpool_ac
}  // namespace esphome
