import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir
from esphome.const import CONF_ID, CONF_MODEL
from esphome.components.homeassistant import binary_sensor

AUTO_LOAD = ["climate_ir", "binary_sensor"]
CODEOWNERS = ["@glmnet"]

whirlpool_ac_ns = cg.esphome_ns.namespace("whirlpool_ac")
WhirlpoolClimateAC = whirlpool_ac_ns.class_("WhirlpoolClimateAC", climate_ir.ClimateIR)

Model = whirlpool_ac_ns.enum("Model")
MODELS = {
    "DG11J1-3A": Model.MODEL_DG11J1_3A,
    "DG11J1-91": Model.MODEL_DG11J1_91,
}

CONF_IR_TRANSMITTER_MUTE = "ir_transmitter_mute"

CONFIG_SCHEMA = climate_ir.CLIMATE_IR_WITH_RECEIVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(WhirlpoolClimateAC),
        cv.Optional(CONF_MODEL, default="DG11J1-3A"): cv.enum(MODELS, upper=True),
        cv.Optional(CONF_IR_TRANSMITTER_MUTE): cv.use_id(binary_sensor.HomeassistantBinarySensor),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await climate_ir.register_climate_ir(var, config)
    cg.add(var.set_model(config[CONF_MODEL]))
    if CONF_IR_TRANSMITTER_MUTE in config:
        sens = await cg.get_variable(config[CONF_IR_TRANSMITTER_MUTE])
        cg.add(var.set_ir_transmitter_mute(sens))
