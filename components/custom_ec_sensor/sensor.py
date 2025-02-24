import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID

custom_ec_sensor_ns = cg.esphome_ns.namespace("custom_ec_sensor")

# Declare your C++ class 
EcSensor = custom_ec_sensor_ns.class_("EcSensor", cg.PollingComponent, sensor.Sensor)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(EcSensor),
    cv.Required("ads_sensor"): cv.use_id(sensor.Sensor),
    cv.Required("water_temperature"): cv.use_id(sensor.Sensor)
    
}).extend(sensor.sensor_schema(EcSensor))

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield sensor.register_sensor(var, config)




# Declare your C++ class with both polling and sensor bases.
EcSensor = custom_ec_sensor_ns.class_(
    "EcSensor", cg.PollingComponent, sensor_component.Sensor
)

# Extend the configuration with the default sensor schema.
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(EcSensor),
    cv.Required("ads_sensor"): cv.use_id(sensor_component.Sensor),
    cv.Required("water_temperature"): cv.use_id(sensor_component.Sensor),
}).extend(sensor_component.sensor_schema(EcSensor))



