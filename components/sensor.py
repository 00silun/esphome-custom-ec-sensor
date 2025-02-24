import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor as sensor_component
from esphome.const import CONF_ID

# Create a namespace matching your C++ code.
custom_ec_sensor_ns = cg.esphome_ns.namespace('custom_ec_sensor')

# Declare your C++ class. Ensure "EcSensor" exactly matches your header.
EcSensor = custom_ec_sensor_ns.class_('EcSensor', cg.PollingComponent, sensor_component.sensor)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(EcSensor),
    cv.Required("ads_sensor"): cv.use_id(sensor_component.sensor),
    cv.Required("water_temperature"): cv.use_id(sensor_component.sensor),
}).extend(sensor_component.sensor_schema(EcSensor))
