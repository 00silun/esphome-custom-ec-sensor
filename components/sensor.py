import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

# Delay import of the sensor module to avoid circular imports.
sensor_component = __import__("esphome.components.sensor", fromlist=["Sensor"])

custom_ec_sensor_ns = cg.esphome_ns.namespace("custom_ec_sensor")

# Declare your C++ class using the dynamically imported sensor module.
EcSensor = custom_ec_sensor_ns.class_("EcSensor", cg.PollingComponent, sensor_component.Sensor)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(EcSensor),
    cv.Required("ads_sensor"): cv.use_id(sensor_component.Sensor),
    cv.Required("water_temperature"): cv.use_id(sensor_component.Sensor),
}).extend(sensor_component.sensor_schema(EcSensor))
