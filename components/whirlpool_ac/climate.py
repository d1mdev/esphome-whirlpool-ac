import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir
from esphome.const import CONF_MODEL
from esphome.components import switch

AUTO_LOAD = ["climate_ir", "switch"]
CODEOWNERS = ["@d1mdev"]

whirlpool_ac_ns = cg.esphome_ns.namespace("whirlpool_ac")
WhirlpoolAC = whirlpool_ac_ns.class_("WhirlpoolAC", climate_ir.ClimateIR)
WhirlpoolACSwitch = whirlpool_ac_ns.class_(
    "WhirlpoolACSwitch", switch.Switch, cg.Component
)

Model = whirlpool_ac_ns.enum("Model")
MODELS = {
    "DG11J1-3A": Model.MODEL_DG11J1_3A,
    "DG11J1-91": Model.MODEL_DG11J1_91,
}

CONF_IR_TRANSMITTER_SWITCH = "ir_transmitter_switch"
CONF_IFEEL_SWITCH = "ifeel_switch"

SWITCH_SCHEMA = switch.switch_schema(WhirlpoolACSwitch).extend(cv.COMPONENT_SCHEMA)

CONFIG_SCHEMA = climate_ir.climate_ir_with_receiver_schema(WhirlpoolAC).extend(
    {
        cv.Optional(CONF_MODEL, default="DG11J1-3A"): cv.enum(MODELS, upper=True),
        cv.Optional(CONF_IR_TRANSMITTER_SWITCH): SWITCH_SCHEMA,
        cv.Optional(CONF_IFEEL_SWITCH): SWITCH_SCHEMA,
    }
)

async def to_code(config):
    var = await climate_ir.new_climate_ir(config)
    cg.add(var.set_model(config[CONF_MODEL]))
    for s in [CONF_IR_TRANSMITTER_SWITCH, CONF_IFEEL_SWITCH]:
        if s in config:
            conf = config[s]
            a_switch = await switch.new_switch(conf)
            await cg.register_component(a_switch, conf)
            cg.add(getattr(var, f"set_{s}")(a_switch))
