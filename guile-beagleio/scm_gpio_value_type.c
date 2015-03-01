#include "event_gpio.h"
#include "guile_beagleio_gpio.h"
#include "scm_gpio_value_type.h"

static scm_t_bits gpio_value_tag;

typedef struct gpio_value {
  unsigned int pin_number;
  unsigned int (*sysfs_value)(const void* self);
  const char *view;
  SCM update_func;
} GpioValue;

static const char *HIGH_PRINT = "high";
static const char *LOW_PRINT = "low";

unsigned int
get_sysfs_value(const void* self) {
  unsigned int value;
  GpioValue *me = (GpioValue*)self;
  if (!me->pin_number) {
    if (strcmp(me->view, HIGH_PRINT) == 0) {
      return HIGH;
    } else {
      return LOW;
    }
  } else {
    if (gpio_get_value((unsigned int) me->pin_number, &value) == -1) {
      return scm_gpio_throw("unable to read /sys/class/gpio/*/value");
    }
  }
  return value;
}

static int
scm_gpio_value_print(SCM gpio_value_smob, SCM port, scm_print_state *pstate) {
  GpioValue *gpio_value;
  scm_assert_smob_type(gpio_value_tag, gpio_value_smob);
  gpio_value = (GpioValue*)SCM_SMOB_DATA(gpio_value_smob);
  scm_puts("#<gpio-value level: ", port);
  if (gpio_value->sysfs_value(gpio_value) == HIGH) {
    scm_puts(HIGH_PRINT, port);
  } else {
    scm_puts(LOW_PRINT, port);
  }
  scm_puts(">", port);
  return 1;
}

static size_t
scm_gpio_value_free(SCM gpio_value_smob) {
  GpioValue *gpio_value;
  scm_assert_smob_type(gpio_value_tag, gpio_value_smob);
  gpio_value = (GpioValue*) SCM_SMOB_DATA(gpio_value_smob);
  scm_gc_free(gpio_value, sizeof(GpioValue), "gpio-value");
  return 0;
}

static SCM
scm_gpio_value_equalp(SCM gpio_value_smob, SCM other_gpio_value_smob) {
  GpioValue *gpio_value, *other_gpio_value;
  scm_assert_smob_type(gpio_value_tag, gpio_value_smob);
  scm_assert_smob_type(gpio_value_tag, other_gpio_value_smob);
  gpio_value = (GpioValue*) SCM_SMOB_DATA(gpio_value_smob);
  other_gpio_value = (GpioValue*) SCM_SMOB_DATA(other_gpio_value_smob);
  if (gpio_value->sysfs_value(gpio_value) == other_gpio_value->sysfs_value(other_gpio_value)) {
    return SCM_BOOL_T;
  }
  return SCM_BOOL_F;
}

SCM
scm_new_gpio_value_smob(unsigned int *pin_number) {
  GpioValue *gpio_value;
  gpio_value = (GpioValue*) scm_gc_malloc(sizeof(GpioValue), "gpio-value");
  gpio_value->pin_number = *pin_number;
  gpio_value->update_func = SCM_BOOL_F;
  gpio_value->sysfs_value = &get_sysfs_value;
  gpio_value->view = HIGH_PRINT;
  return scm_new_smob(gpio_value_tag, (scm_t_bits) gpio_value);
}

static SCM
make_default_value(const char *view) {
  GpioValue *gpio_value;
  gpio_value = (GpioValue *) scm_gc_malloc(sizeof(GpioValue), "gpio-value");
  gpio_value->update_func = SCM_BOOL_F;
  gpio_value->sysfs_value = &get_sysfs_value;
  gpio_value->view = view;
  return scm_new_smob(gpio_value_tag, (scm_t_bits) gpio_value);
}

SCM
scm_gpio_value_high_smob(void) {
  return make_default_value(HIGH_PRINT);
}

SCM
scm_gpio_value_low_smob(void) {
  return make_default_value(LOW_PRINT);
}

void
init_gpio_value_type(void) {
  gpio_value_tag = scm_make_smob_type("gpio-value", sizeof(GpioValue));
  scm_set_smob_print(gpio_value_tag, scm_gpio_value_print);
  scm_set_smob_free(gpio_value_tag, scm_gpio_value_free);
  scm_set_smob_equalp(gpio_value_tag, scm_gpio_value_equalp);
}

void
gpio_value_smob_to_bbio_value(SCM *value_smob, int *value) {
  GpioValue *gpio_value;
  scm_assert_smob_type(gpio_value_tag, *value_smob);
  gpio_value = (GpioValue *) SCM_SMOB_DATA (*value_smob);
  if (!gpio_value->pin_number) {
    if (strcmp(gpio_value->view, HIGH_PRINT) == 0) {
      *value = HIGH;
    } else {
      *value = LOW;
    }
  } else {
    *value = gpio_value->sysfs_value(gpio_value);
  }
}
