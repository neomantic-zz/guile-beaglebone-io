#include "event_gpio.h"
#include "guile_beagleio_gpio.h"
#include "scm_gpio_level_type.h"

static scm_t_bits gpio_level_tag;

typedef struct gpio_level {
  unsigned int pin_number;
  unsigned int (*sysfs_value)(const void* self);
  const char *view;
  SCM update_func;
} GpioLevel;

static const char *HIGH_PRINT = "HIGH";
static const char *LOW_PRINT = "LOW";

unsigned int
get_sysfs_value(const void* self) {
  unsigned int level;
  GpioLevel *me = (GpioLevel*)self;
  if (!me->pin_number) {
    if (strcmp(me->view, HIGH_PRINT) == 0) {
      return HIGH;
    } else {
      return LOW;
    }
  } else {
    if (gpio_get_value((unsigned int) me->pin_number, &level) == -1) {
      return scm_gpio_throw("unable to read /sys/class/gpio/*/value");
    }
  }
  return level;
}

static int
scm_gpio_level_print(SCM gpio_level_smob, SCM port, scm_print_state *pstate) {
  GpioLevel *gpio_level;
  scm_assert_smob_type(gpio_level_tag, gpio_level_smob);
  gpio_level = (GpioLevel*)SCM_SMOB_DATA(gpio_level_smob);
  scm_puts("#<gpio-level ", port);
  if (gpio_level->sysfs_value(gpio_level) == HIGH) {
    scm_puts(HIGH_PRINT, port);
  } else {
    scm_puts(LOW_PRINT, port);
  }
  scm_puts(">", port);
  return 1;
}

static size_t
scm_gpio_level_free(SCM gpio_level_smob) {
  GpioLevel *gpio_level;
  scm_assert_smob_type(gpio_level_tag, gpio_level_smob);
  gpio_level = (GpioLevel*) SCM_SMOB_DATA(gpio_level_smob);
  scm_gc_free(gpio_level, sizeof(GpioLevel), "gpio-level");
  return 0;
}

static SCM
scm_gpio_level_equalp(SCM gpio_level_smob, SCM other_gpio_level_smob) {
  GpioLevel *gpio_level, *other_gpio_level;
  scm_assert_smob_type(gpio_level_tag, gpio_level_smob);
  scm_assert_smob_type(gpio_level_tag, other_gpio_level_smob);
  gpio_level = (GpioLevel*) SCM_SMOB_DATA(gpio_level_smob);
  other_gpio_level = (GpioLevel*) SCM_SMOB_DATA(other_gpio_level_smob);
  if (gpio_level->sysfs_value(gpio_level) == other_gpio_level->sysfs_value(other_gpio_level)) {
    return SCM_BOOL_T;
  }
  return SCM_BOOL_F;
}

SCM
scm_new_gpio_level_smob(unsigned int *pin_number) {
  GpioLevel *gpio_level;
  gpio_level = (GpioLevel*) scm_gc_malloc(sizeof(GpioLevel), "gpio-level");
  gpio_level->pin_number = *pin_number;
  gpio_level->update_func = SCM_BOOL_F;
  gpio_level->sysfs_value = &get_sysfs_value;
  gpio_level->view = HIGH_PRINT;
  return scm_new_smob(gpio_level_tag, (scm_t_bits) gpio_level);
}

static SCM
make_default_level(const char *view) {
  GpioLevel *gpio_level;
  gpio_level = (GpioLevel *) scm_gc_malloc(sizeof(GpioLevel), "gpio-level");
  gpio_level->update_func = SCM_BOOL_F;
  gpio_level->sysfs_value = &get_sysfs_value;
  gpio_level->view = view;
  return scm_new_smob(gpio_level_tag, (scm_t_bits) gpio_level);
}

SCM
scm_gpio_level_high_smob(void) {
  return make_default_level(HIGH_PRINT);
}

SCM
scm_gpio_level_low_smob(void) {
  return make_default_level(LOW_PRINT);
}

void
init_gpio_level_type(void) {
  gpio_level_tag = scm_make_smob_type("gpio-level", sizeof(GpioLevel));
  scm_set_smob_print(gpio_level_tag, scm_gpio_level_print);
  scm_set_smob_free(gpio_level_tag, scm_gpio_level_free);
  scm_set_smob_equalp(gpio_level_tag, scm_gpio_level_equalp);
}

void
level_smob_to_bbio_value(SCM *level_smob, int *level) {
  GpioLevel *gpio_level;
  scm_assert_smob_type(gpio_level_tag, *level_smob);
  gpio_level = (GpioLevel *) SCM_SMOB_DATA (*level_smob);
  if (!gpio_level->pin_number) {
    if (strcmp(gpio_level->view, HIGH_PRINT) == 0) {
      *level = HIGH;
    } else {
      *level = LOW;
    }
  } else {
    *level = gpio_level->sysfs_value(gpio_level);
  }
}
