# esphome-whirlpool-ac

Fork of standart [esphome/components/whirlpool](https://github.com/esphome/esphome/tree/28b5c535ec597571d836aeaf2c051a3c0b61b976/esphome/components/whirlpool) with additions:
* ~~add 1 sec mute pause for receiver just after ir transmitter send command to climate (this prevent component switching twice);~~
* ~~add support for optional binary sensor, that mute transmitter (it can help sync state of Home Assistant climate card with real climate state if they're out of sync);~~
* replace binary sensor with switch component, that on (by default) and off transmitter (it can help sync state of Home Assistant climate card with real climate state if they're out of sync);
* add support for iFeel mode (transmit to HVAc temperature from external source);

# Alert! New syntax of esphome yaml (see example.yaml)

# Esphome module config:
```
#Add this to turn on transmitter on esp boot
esphome:
  on_boot: 
    then:
      switch.turn_on: ir_transmitter_switch_id

external_components:
  source: github://d1mdev/esphome-whirlpool-ac
  components: [whirlpool_ac]

remote_receiver:
  id: rcvr_hvac
  pin:

remote_transmitter:
  pin:

sensor:
  - platform: whatever
    name: "Room temperature"
    entity_id: sensor.XXXXXXXXXXX_temperature
    id: room_temperature
    internal: true
    unit_of_measurement: '°C'
    filters:
      - throttle: 1s
      - heartbeat: 30s
      - debounce: 0.5s

climate:
  - platform: whirlpool_ac
    id: climate_id
    name: "climate name"
    receiver_id: <rcvr_id>
    sensor: <temperature_sensor_id>
    ir_transmitter_switch:
      name: Transmitter
      id: ir_transmitter_switch_id
      restore_mode: ALWAYS_ON
    ifeel_switch:
      name: iFeel
    supports_cool: True
    supports_heat: True
    model: DG11J1-91
    visual:
      min_temperature: 16
      max_temperature: 30
      temperature_step: 1
```
# to-do
* ~~Implement wired powered_on_assumed~~
