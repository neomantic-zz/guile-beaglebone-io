#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "event_gpio.h"
#include "guile_beagleio_gpio.h"
#include "scm_gpio_type.h"
#include "scm_gpio_setting_type.h"
#include "scm_gpio_value_type.h"
#include "scm_gpio_direction_type.h"

SCM
scm_gpio_throw(char *message)
{
  return scm_throw(scm_from_utf8_symbol("gpio-error"), scm_list_1(scm_from_utf8_string(message)));
}

SCM
lookup_gpio_number(SCM s_channel)
{
  unsigned int gpio_number;
  char *channel = scm_to_locale_string(s_channel);
  get_gpio_number(channel, &gpio_number);
  if(!gpio_number)
    return scm_from_int(0);

  return scm_from_ulong(gpio_number);
}

SCM
setup_channel(SCM s_channel)
{
  unsigned int gpio_number;
  Gpio *gpio;
  SCM gpio_smob;

  get_gpio_number(scm_to_locale_string(s_channel), &gpio_number);

  if (!gpio_number)
    return scm_gpio_throw("unable to find pin number");

  if (gpio_export(gpio_number) != 0 )
    return scm_gpio_throw("unable to export to /sys/class/gpio");

  gpio_smob = scm_new_gpio_smob(&gpio_number, &s_channel);
  gpio = (Gpio *)SCM_SMOB_DATA(gpio_smob);
  gpio->setDirection(gpio, OUTPUT);
  gpio_set_value(gpio_number, LOW);

  return gpio_smob;
}

SCM
set_direction(SCM gpio_smob, SCM gpio_direction_smob)
{
  Gpio *gpio;
  GpioDirection *gpio_direction;
  int success;

  scm_assert_gpio_smob_type(&gpio_smob);
  scm_assert_gpio_direction_smob(&gpio_direction_smob);

  gpio_direction = (GpioDirection *)SCM_SMOB_DATA(gpio_direction_smob);
  gpio = (Gpio *) SCM_SMOB_DATA(gpio_smob);

  success = gpio->setDirection(gpio, gpio_direction->bbio_value);
  if (success == 0)
      return gpio_smob;

  if (success != -1)
      return scm_gpio_throw("only accepts INPUT and OUTPUT");
  return scm_gpio_throw("unable to write to /sys/class/gpio");

}

SCM
get_direction(SCM gpio_smob)
{
  Gpio *gpio;
  unsigned int direction;
  int success;
  scm_assert_gpio_smob_type(&gpio_smob);
  gpio = (Gpio *) SCM_SMOB_DATA(gpio_smob);
  success = gpio->direction(gpio, &direction);
  if (success == 0)
    return scm_new_gpio_direction_smob(direction);

  if (success != -1)
    return scm_gpio_throw("unable to write /sys/class/gpio/*/direction to reset it");
  return scm_gpio_throw("unable to read /sys/class/gpio/*/direction");
}

SCM
gpio_cleanup(void)
{
  event_cleanup();
  return SCM_UNDEFINED;
}

SCM
set_value(SCM gpio_smob, SCM gpio_value_smob)
{
  Gpio *gpio;
  GpioValue *gpio_value;
  int success;
  unsigned int current_direction;
  scm_assert_gpio_smob_type(&gpio_smob);
  scm_assert_gpio_value_smob(&gpio_value_smob);
  gpio = (Gpio *) SCM_SMOB_DATA(gpio_smob);

  success = gpio->direction(gpio, &current_direction);
  if (success != 0) {
    if (success == -1)
      return scm_gpio_throw("unable to read /sys/class/gpio/*/direction");
    return scm_gpio_throw("unable to write /sys/class/gpio/*/direction to reset it");
  }

  if(current_direction != OUTPUT)
    return scm_gpio_throw("The gpio channel has not been setup as output");

  gpio_value = (GpioValue *)SCM_SMOB_DATA(gpio_value_smob);
  if(gpio_set_value(gpio->pin_number, gpio_value->bbio_value) == -1)
    return scm_gpio_throw("unable to read /sys/class/gpio");

  return gpio_smob;
}

SCM
get_value(SCM gpio_smob)
{
  Gpio *gpio;
  unsigned int value;
  scm_assert_gpio_smob_type(&gpio_smob);
  gpio = (Gpio *) SCM_SMOB_DATA (gpio_smob);
  if(gpio_get_value(gpio->pin_number, &value) == -1)
    return scm_gpio_throw("unable to read /sys/class/gpio/*/value");

  return scm_new_gpio_value_smob(value);
}

void
scm_init_beagleio_gpio(void)
{
  static int initialized = 0;
  if (initialized)
    return;

  init_gpio_type();
  init_gpio_value_type();
  init_gpio_direction_type();
  scm_c_define_gsubr("gpio-setup", 1, 0, 0, setup_channel);
  scm_c_define_gsubr("gpio-cleanup-all", 0, 0, 0, gpio_cleanup);
  scm_c_define_gsubr("gpio-direction-set!", 2, 0, 0, set_direction);
  scm_c_define_gsubr("gpio-direction", 1, 0, 0, get_direction);
  scm_c_define_gsubr("gpio-number-lookup", 1, 0, 0, lookup_gpio_number);
  scm_c_define_gsubr("gpio?", 1, 0, 0, scm_gpio_type_p);
  scm_c_define("INPUT", scm_new_gpio_direction_smob(INPUT));
  scm_c_define("OUTPUT", scm_new_gpio_direction_smob(OUTPUT));
  scm_c_define_gsubr("gpio-value-set!", 2, 0, 0, set_value);
  scm_c_define_gsubr("gpio-value", 1, 0, 0, get_value);
  scm_c_define("HIGH", scm_new_gpio_value_smob(HIGH));
  scm_c_define("LOW", scm_new_gpio_value_smob(LOW));

  initialized = 1;
}
