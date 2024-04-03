#pragma once

#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome {
namespace whirlpool_ac {

class WhirlpoolACSwitch : public switch_::Switch, public Component {
 protected:
  void write_state(bool state) override { this->publish_state(state); }
};

}  // namespace whirlpool_ac
}  // namespace esphome
