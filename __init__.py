import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

DEPENDENCIES = ['i2c']
AUTO_LOAD = ['sensor']
MULTI_CONF = True

CONF_HEATER_TEMPERATURE = 'heater_temperature'
CONF_HEATER_DURATION = 'heater_duration'
CONF_IIR_FILTER_COEFFICIENT = 'iir_filter_coefficient'
CONF_TEMPERATURE_OVERSAMPLING = 'temperature_oversampling'
CONF_PRESSURE_OVERSAMPLING = 'pressure_oversampling'
CONF_HUMIDITY_OVERSAMPLING = 'humidity_oversampling'

bme690_ns = cg.esphome_ns.namespace('bme690')
BME690Component = bme690_ns.class_(
    'BME690Component', cg.PollingComponent, i2c.I2CDevice
)

# Oversampling options matching BME69X API
OVERSAMPLING_OPTIONS = {
    'NONE': 0,    # BME69X_OS_NONE
    '1X': 1,      # BME69X_OS_1X
    '2X': 2,      # BME69X_OS_2X
    '4X': 3,      # BME69X_OS_4X
    '8X': 4,      # BME69X_OS_8X
    '16X': 5,     # BME69X_OS_16X
}

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(BME690Component),
    cv.Optional(CONF_HEATER_TEMPERATURE, default=320): cv.int_range(min=200, max=400),
    cv.Optional(CONF_HEATER_DURATION, default=150): cv.int_range(min=1, max=4032),
    cv.Optional(CONF_TEMPERATURE_OVERSAMPLING, default='8X'): cv.enum(OVERSAMPLING_OPTIONS, upper=True),
    cv.Optional(CONF_PRESSURE_OVERSAMPLING, default='4X'): cv.enum(OVERSAMPLING_OPTIONS, upper=True),
    cv.Optional(CONF_HUMIDITY_OVERSAMPLING, default='2X'): cv.enum(OVERSAMPLING_OPTIONS, upper=True),
    cv.Optional(CONF_IIR_FILTER_COEFFICIENT, default=3): cv.int_range(min=0, max=127),
}).extend(cv.polling_component_schema('60s')).extend(i2c.i2c_device_schema(0x77))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    
    cg.add(var.set_heater_temperature(config[CONF_HEATER_TEMPERATURE]))
    cg.add(var.set_heater_duration(config[CONF_HEATER_DURATION]))
    cg.add(var.set_temperature_oversampling(config[CONF_TEMPERATURE_OVERSAMPLING]))
    cg.add(var.set_pressure_oversampling(config[CONF_PRESSURE_OVERSAMPLING]))
    cg.add(var.set_humidity_oversampling(config[CONF_HUMIDITY_OVERSAMPLING]))
    cg.add(var.set_iir_filter(config[CONF_IIR_FILTER_COEFFICIENT]))
    
    # Add the BME69x API source files to the build
    # These files should be in the same directory as this __init__.py
    cg.add_build_flag('-I' + str(cg.relative_src_path('components/bme690')))
    cg.add_library_file('bme69x.c')
