project(I2C_APP C)

# Create the app module
add_cfe_app(i2c_app fsw/src/i2c_app.c)

# Add table
add_cfe_tables(i2c_app fsw/tables/sample_app_tbl.c)

target_include_directories(i2c_app PUBLIC
  fsw/mission_inc
  fsw/platform_inc
)

# If UT is enabled, then add the tests from the subdirectory
# Note that this is an app, and therefore does not provide
# stub functions, as other entities would not typically make 
# direct function calls into this application.
if (ENABLE_UNIT_TESTS)
  add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)
