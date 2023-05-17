# esphome-whirlpool-ac

# Esphome module config:
```
external_components:
  source: github://d1mdev/esphome-whirlpool-ac
  components: [whirlpool_ac]

# YAML
climate:
  - platform: whirlpool_ac
    id: climate_id
    name: "climate name"
    receiver_id: <rcvr_id>
    sensor: <temperature_sensor_id>
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
