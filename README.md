# esphome-whirlpool-ac

Fork of standart [esphome/components/whirlpool](https://github.com/esphome/esphome/tree/28b5c535ec597571d836aeaf2c051a3c0b61b976/esphome/components/whirlpool) with additions:
* add 1 sec mute pause for receiver just after ir transmitter send command to climate (this prevent component switching twice);
* add support for optional binary sensor, that mute transmitter (it can help sync state of Home Assistant climate card with real climate state if they're out of sync); 

# Esphome module config:
```
external_components:
  source: github://d1mdev/esphome-whirlpool-ac
  components: [whirlpool_ac]

climate:
  - platform: whirlpool_ac
    id: climate_id
    name: "climate name"
    receiver_id: <rcvr_id>
    sensor: <temperature_sensor_id>
    # optional
    ir_transmitter_mute: <binary_sensor_id>
    supports_cool: True
    supports_heat: True
    model: DG11J1-91
    visual:
      min_temperature: 16
      max_temperature: 30
      temperature_step: 1
```
# to-do
* Implement wired powered_on_assumed
