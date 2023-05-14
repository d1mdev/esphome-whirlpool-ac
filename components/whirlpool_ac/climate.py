import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir
from esphome.const import CONF_ID, CONF_MODEL, CONF_SENSOR_ID

AUTO_LOAD = ["climate_ir"]
CODEOWNERS = ["@glmnet"]

whirlpool_ac_ns = cg.esphome_ns.namespace("whirlpool_ac")
WhirlpoolClimateAC = whirlpool_ac_ns.class_("WhirlpoolClimateAC", climate_ir.ClimateIR)

Model = whirlpool_ac_ns.enum("Model")
MODELS = {
    "DG11J1-3A": Model.MODEL_DG11J1_3A,
    "DG11J1-91": Model.MODEL_DG11J1_91,
}

CONFIG_SCHEMA = climate_ir.CLIMATE_IR_WITH_RECEIVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(WhirlpoolClimateAC),
        cv.Optional(CONF_MODEL, default="DG11J1-3A"): cv.enum(MODELS, upper=True),
        cv.Optional(CONF_SENSOR_ID): cv.use_id(sensor.Sensor),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await climate_ir.register_climate_ir(var, config)
    cg.add(var.set_model(config[CONF_MODEL]))
    if CONF_SENSOR_ID in config:
        sens = await cg.get_variable(config[CONF_SENSOR])
        cg.add(var.set_sensor(sens))
