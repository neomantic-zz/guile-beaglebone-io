#include <stdlib.h>
#include <string.h>
#include <libguile.h>
#include "common.h"
#include "event_gpio.h"
#include "guile_beagleio_gpio.h"

static scm_t_bits gpio_tag;

struct gpio {
  unsigned int pin_number;
  SCM channel;
  SCM update_func;
};

static scm_t_bits gpio_level_tag;

struct gpio_level {
  int bbio_level;
  SCM update_func;
};

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
  SCM smob;
  struct gpio *gpio;
  unsigned int gpio_number;
  int exported;
  char *channel = scm_to_locale_string(s_channel);

  get_gpio_number(channel, &gpio_number);

  if (!gpio_number) {
    return scm_throw(scm_from_utf8_symbol("gpio-error"), scm_list_1(scm_from_utf8_string("unable to find pin number")));
  }

  exported = gpio_export(gpio_number);
  if (exported != 0 ) {
    return scm_throw(scm_from_utf8_symbol("gpio-error"), scm_list_1(scm_from_utf8_string("unable to export to /sys/class/gpio")));
  }

  gpio = (struct gpio *) scm_gc_malloc(sizeof(struct gpio), "gpio");
  gpio->pin_number = gpio_number;
  gpio->channel = SCM_BOOL_F;
  gpio->update_func = SCM_BOOL_F;

  smob = scm_new_smob(gpio_tag, (scm_t_bits) gpio);
  gpio->channel = s_channel;
  return smob;
}

static int
print_gpio(SCM gpio_smob, SCM port, scm_print_state *pstate) {
  struct gpio *gpio;
  scm_assert_smob_type(gpio_tag, gpio_smob);
  gpio = (struct gpio*) SCM_SMOB_DATA(gpio_smob);
  scm_puts("#<gpio channel=", port);
  scm_display(gpio->channel, port);
  scm_puts(" pin=", port);
  scm_display(scm_from_unsigned_integer(gpio->pin_number), port);
  scm_puts(">", port);
  return 1;
}

static size_t
free_gpio(SCM gpio_smob) {
  scm_assert_smob_type(gpio_tag, gpio_smob);
  struct gpio *gpio = (struct gpio *) SCM_SMOB_DATA(gpio_smob);
  scm_gc_free(gpio, sizeof(struct gpio), "gpio");
  return 0;
}

static SCM
mark_gpio(SCM gpio_smob) {
  scm_assert_smob_type(gpio_tag, gpio_smob);
  struct gpio *gpio = (struct gpio *) SCM_SMOB_DATA (gpio_smob);
  scm_gc_mark(gpio->channel);
  return (SCM) gpio->update_func;
}

static SCM
equal_gpio(SCM gpio_smob, SCM other_gpio_smob ){
  scm_assert_smob_type(gpio_tag, gpio_smob);
  scm_assert_smob_type(gpio_tag, other_gpio_smob);
  struct gpio *gpio = (struct gpio *) SCM_SMOB_DATA (gpio_smob);
  struct gpio *other = (struct gpio *) SCM_SMOB_DATA (other_gpio_smob);
  if ( gpio->pin_number == other->pin_number){
      return SCM_BOOL_T;
  }
  return SCM_BOOL_F;
}

void
init_gpio_type(void) {
  gpio_tag = scm_make_smob_type("gpio", sizeof(struct gpio));
  scm_set_smob_print(gpio_tag, print_gpio);
  scm_set_smob_free(gpio_tag, free_gpio);
  scm_set_smob_mark(gpio_tag, mark_gpio);
  scm_set_smob_equalp(gpio_tag, equal_gpio);
}

static int
print_gpio_level(SCM gpio_level_smob, SCM port, scm_print_state *pstate) {
  struct gpio_level *gpio_level;
  scm_assert_smob_type(gpio_level_tag, gpio_level_smob);
  gpio_level = (struct gpio_level*) SCM_SMOB_DATA(gpio_level_smob);
  scm_puts("#<gpio-level ", port);
  if (gpio_level->bbio_level == HIGH) {
    scm_puts("HIGH", port);
  } else {
    scm_puts("LOW", port);
  }
  scm_puts(">", port);
  return 1;
}

static size_t
free_gpio_level(SCM gpio_level_smob) {
  struct gpio_level *gpio_level;
  scm_assert_smob_type(gpio_level_tag, gpio_level_smob);
  gpio_level = (struct gpio_level*) SCM_SMOB_DATA(gpio_level_smob);
  scm_gc_free(gpio_level, sizeof(struct gpio_level), "gpio-level");
  return 0;
}

static SCM
gpio_level_equalp(SCM gpio_level_smob, SCM other_gpio_level_smob) {
  struct gpio_level *gpio_level;
  struct gpio_level *other_gpio_level;
  scm_assert_smob_type(gpio_level_tag, gpio_level_smob);
  scm_assert_smob_type(gpio_level_tag, other_gpio_level_smob);
  gpio_level = (struct gpio_level*) SCM_SMOB_DATA(gpio_level_smob);
  other_gpio_level = (struct gpio_level*) SCM_SMOB_DATA(other_gpio_level_smob);
  if (gpio_level->bbio_level == other_gpio_level->bbio_level) {
    return SCM_BOOL_T;
  }
  return SCM_BOOL_F;
}

static void init_gpio_level_type(void) {
  gpio_level_tag = scm_make_smob_type("gpio-level", sizeof(struct gpio_level));
  scm_set_smob_print(gpio_level_tag, print_gpio_level);
  scm_set_smob_free(gpio_level_tag, free_gpio_level);
  scm_set_smob_equalp(gpio_level_tag, gpio_level_equalp);
}

static SCM make_gpio_level(const int level) {
  struct gpio_level *gpio_level;
  gpio_level = (struct gpio_level *) scm_gc_malloc(sizeof(struct gpio_level), "gpio-level");
  gpio_level->bbio_level = level;
  gpio_level->update_func = SCM_BOOL_F;
  return scm_new_smob(gpio_level_tag, (scm_t_bits) gpio_level);
}

#define HIGH_SMOB make_gpio_level(HIGH)
#define LOW_SMOB make_gpio_level(LOW)

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
    return scm_throw(scm_from_utf8_symbol("gpio-error"), scm_list_1(scm_from_utf8_string("only accepts INPUT and OUTPUT")));
  }

  if (success == -1 ) {
    return scm_throw(scm_from_utf8_symbol("gpio-error"), scm_list_1(scm_from_utf8_string("unable to write to /sys/class/gpio")));
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
    return scm_throw(scm_from_utf8_symbol("gpio-error"), scm_list_1(scm_from_utf8_string("unable to read /sys/class/gpio")));
  }
  return scm_from_int(value);
}

SCM
gpio_predicate(SCM smob) {
  if (!SCM_SMOB_PREDICATE(gpio_tag, smob)) {
    return SCM_BOOL_F;
  }
  return SCM_BOOL_T;
}

SCM
gpio_cleanup() {
  event_cleanup();
  return SCM_UNDEFINED;
}

int
get_level_smob_value(SCM *level_smob, int *level) {
  struct gpio_level *gpio_level;
  scm_assert_smob_type(gpio_level_tag, *level_smob);
  gpio_level = (struct gpio_level *) SCM_SMOB_DATA (*level_smob);
  *level = gpio_level->bbio_level;
  return 0;
}

SCM
set_value(SCM gpio_smob, SCM level_smob) {
  struct gpio *gpio;
  int level;
  scm_assert_smob_type(gpio_tag, gpio_smob);
  get_level_smob_value(&level_smob, &level);
  gpio = (struct gpio *) SCM_SMOB_DATA (gpio_smob);
  if( gpio_set_value(gpio->pin_number, (unsigned int) level) == -1) {
    return scm_throw(scm_from_utf8_symbol("gpio-error"), scm_list_1(scm_from_utf8_string("unable to read /sys/class/gpio")));
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
    return scm_throw(scm_from_utf8_symbol("gpio-error"), scm_list_1(scm_from_utf8_string("unable to read /sys/class/gpio/*/value")));
  }
  if (value == HIGH ) {
    return HIGH_SMOB;
  } else {
    return LOW_SMOB;
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
  scm_c_define_gsubr("gpio?", 1, 0, 0, gpio_predicate);
  scm_c_define("INPUT", scm_from_int(INPUT));
  scm_c_define("OUTPUT", scm_from_int(OUTPUT));
  scm_c_define_gsubr("gpio-value-set!", 2, 0, 0, set_value);
  scm_c_define_gsubr("gpio-value", 1, 0, 0, get_value);
  scm_c_define("HIGH", HIGH_SMOB);
  scm_c_define("LOW", LOW_SMOB);

  initialized = 1;
}
