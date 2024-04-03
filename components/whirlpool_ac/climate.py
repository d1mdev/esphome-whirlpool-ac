import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir
from esphome.const import CONF_ID, CONF_MODEL
from esphome.components import switch

AUTO_LOAD = ["climate_ir", "switch"]
CODEOWNERS = ["@d1mdev"]

whirlpool_ac_ns = cg.esphome_ns.namespace("whirlpool_ac")
WhirlpoolClimateAC = whirlpool_ac_ns.class_("WhirlpoolClimateAC", climate_ir.ClimateIR)
WhirlpoolClimateACSwitch = whirlpool_ac_ns.class_(
    "WhirlpoolClimateACSwitch", switch.Switch, cg.Component
)

Model = whirlpool_ac_ns.enum("Model")
MODELS = {
    "DG11J1-3A": Model.MODEL_DG11J1_3A,
    "DG11J1-91": Model.MODEL_DG11J1_91,
}

CONF_IR_TRANSMITTER_SWITCH = "ir_transmitter_switch"
CONF_IFEEL_SWITCH = "ifeel_switch"

SWITCH_SCHEMA = switch.SWITCH_SCHEMA.extend(cv.COMPONENT_SCHEMA).extend(
    {cv.GenerateID(): cv.declare_id(WhirlpoolClimateACSwitch)}
)

CONFIG_SCHEMA = climate_ir.CLIMATE_IR_WITH_RECEIVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(WhirlpoolClimateAC),
        cv.Optional(CONF_MODEL, default="DG11J1-3A"): cv.enum(MODELS, upper=True),
        cv.Optional(CONF_IR_TRANSMITTER_SWITCH): SWITCH_SCHEMA,
        cv.Optional(CONF_IFEEL_SWITCH): SWITCH_SCHEMA,
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await climate_ir.register_climate_ir(var, config)
    cg.add(var.set_model(config[CONF_MODEL]))
    for s in [CONF_IR_TRANSMITTER_SWITCH, CONF_IFEEL_SWITCH]:
        if s in config:
            conf = config[s]
            a_switch = cg.new_Pvariable(conf[CONF_ID])
            await cg.register_component(a_switch, conf)
            await switch.register_switch(a_switch, conf)
            cg.add(getattr(var, f"set_{s}")(a_switch))
