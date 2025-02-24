import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID

custom_ec_sensor_ns = cg.esphome_ns.namespace("custom_ec_sensor")

# Declare your C++ class 
EcSensor = custom_ec_sensor_ns.class_("EcSensor", cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(EcSensor),
    cv.Required("ads_sensor"): cv.use_id(sensor.Sensor),
    cv.Required("water_temperature"): cv.use_id(sensor.Sensor)
    
}).extend(cv.polling_component_schema('10s'))

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield sensor.register_sensor(var, config)






