#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "event_gpio.h"
#include "guile_beagleio_gpio.h"
#include "scm_gpio_type.h"
#include "scm_gpio_level_type.h"

static SCM
scm_gpio_throw(char *message) {
  return scm_throw(scm_from_utf8_symbol("gpio-error"), scm_list_1(scm_from_utf8_string(message)));
}

SCM
lookup_gpio_number(SCM s_channel) {
  unsigned int gpio_number;
  char *channel = scm_to_locale_string(s_channel);
  get_gpio_number(channel, &gpio_number);
  if(!gpio_number) {
    return scm_from_int(0);
  }
  return scm_from_ulong(gpio_number);
}

SCM
setup_channel(SCM s_channel) {
  unsigned int gpio_number;
  get_gpio_number(scm_to_locale_string(s_channel), &gpio_number);

  if (!gpio_number) {
    return scm_gpio_throw("unable to find pin number");
  }

  if (gpio_export(gpio_number) != 0 ) {
    return scm_gpio_throw("unable to export to /sys/class/gpio");
  }

  return scm_new_gpio_smob(&gpio_number, &s_channel);
}

SCM
set_direction(SCM gpio_smob, SCM pud) {
  struct gpio *gpio;
  scm_assert_smob_type(gpio_tag, gpio_smob);
  gpio = (struct gpio *) SCM_SMOB_DATA (gpio_smob);
  int pud_int = scm_to_int(pud);
  int success;
  if ( pud_int == INPUT ) {
    success = gpio_set_direction(gpio->pin_number, pud_int);
  } else if ( pud_int == OUTPUT ){
    success = gpio_set_direction(gpio->pin_number, pud_int);
  } else {
    return scm_gpio_throw("only accepts INPUT and OUTPUT");
  }

  if (success == -1 ) {
    return scm_gpio_throw("unable to write to /sys/class/gpio");
  }

  return gpio_smob;
}

SCM
get_direction(SCM gpio_smob) {
  struct gpio *gpio;
  unsigned int value;
  scm_assert_smob_type(gpio_tag, gpio_smob);
  gpio = (struct gpio *) SCM_SMOB_DATA (gpio_smob);
  if (gpio_get_direction(gpio->pin_number, &value) == -1) {
    return scm_gpio_throw("unable to read /sys/class/gpio");
  }
  return scm_from_int(value);
}


SCM
gpio_cleanup() {
  event_cleanup();
  return SCM_UNDEFINED;
}

SCM
set_value(SCM gpio_smob, SCM level_smob) {
  struct gpio *gpio;
  int level;
  scm_assert_smob_type(gpio_tag, gpio_smob);
  get_level_smob_value(&level_smob, &level);
  gpio = (struct gpio *) SCM_SMOB_DATA (gpio_smob);

  if( gpio_set_value(gpio->pin_number,(unsigned int) level) == -1) {
    return scm_gpio_throw("unable to read /sys/class/gpio");
  }
  return gpio_smob;
}

SCM
get_value(SCM gpio_smob) {
  struct gpio *gpio;
  unsigned int value;
  scm_assert_smob_type(gpio_tag, gpio_smob);
  gpio = (struct gpio *) SCM_SMOB_DATA (gpio_smob);
  if( gpio_get_value(gpio->pin_number, &value) == -1) {
    return scm_gpio_throw("unable to read /sys/class/gpio/*/value");
  }
  if (value == HIGH) {
    return high_smob();
  } else {
    return low_smob();
  }
}


void
scm_init_beagleio_gpio(void) {
  static int initialized = 0;
  if (initialized)
    return;

  init_gpio_type();
  init_gpio_level_type();
  scm_c_define_gsubr("gpio-setup", 1, 0, 0, setup_channel);
  scm_c_define_gsubr("gpio-cleanup-all", 0, 0, 0, gpio_cleanup);
  scm_c_define_gsubr("gpio-direction-set!", 2, 0, 0, set_direction);
  scm_c_define_gsubr("gpio-direction", 1, 0, 0, get_direction);
  scm_c_define_gsubr("gpio-number-lookup", 1, 0, 0, lookup_gpio_number);
  scm_c_define_gsubr("gpio?", 1, 0, 0, scm_gpio_type_p);
  scm_c_define("INPUT", scm_from_int(INPUT));
  scm_c_define("OUTPUT", scm_from_int(OUTPUT));
  scm_c_define_gsubr("gpio-value-set!", 2, 0, 0, set_value);
  scm_c_define_gsubr("gpio-value", 1, 0, 0, get_value);
  scm_c_define("HIGH", high_smob());
  scm_c_define("LOW", low_smob());
  initialized = 1;
}
