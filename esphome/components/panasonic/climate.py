import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir
from esphome.const import CONF_ID, CONF_MODEL

AUTO_LOAD = ['climate_ir']

panasonic_ns = cg.esphome_ns.namespace('panasonic')
PanasonicClimate = panasonic_ns.class_('PanasonicClimate', climate_ir.ClimateIR)

Model = panasonic_ns.enum('Model')
MODELS = {
    'NZ9SKE': Model.MODEL_NZ9_SKE,
}

CONFIG_SCHEMA = climate_ir.CLIMATE_IR_WITH_RECEIVER_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(PanasonicClimate),
    cv.Optional(CONF_MODEL, default='NZ9SKE'): cv.enum(MODELS, upper=True)
})


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield climate_ir.register_climate_ir(var, config)
    cg.add(var.set_model(config[CONF_MODEL]))
