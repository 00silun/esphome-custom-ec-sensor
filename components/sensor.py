import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, UNIT_EMPTY, ICON_EMPTY

custom_ec_sensor_ns = cg.esphome_ns.namespace("custom_ec_sensor")

# Declare your C++ class using the dynamically imported sensor module.
EcSensor = custom_ec_sensor_ns.class_("EcSensor", cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(EcSensor)
    
}).extend(sensor.sensor_schema('10s'))

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield sensor.register_sensor(var, config)



