#include <libguile.h>
#include "event_gpio.h"
#include "scm_gpio_level_type.h"

static scm_t_bits gpio_level_tag;

struct gpio_level {
  int bbio_level;
  SCM update_func;
};

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

static SCM
gpio_level_smob(int level) {
  struct gpio_level *gpio_level;
  gpio_level = (struct gpio_level *) scm_gc_malloc(sizeof(struct gpio_level), "gpio-level");
  gpio_level->bbio_level = level;
  gpio_level->update_func = SCM_BOOL_F;
  return scm_new_smob(gpio_level_tag, (scm_t_bits) gpio_level);
}

SCM high_smob_value = SCM_BOOL_F;
SCM low_smob_value = SCM_BOOL_F;

SCM
high_smob(void) {
  if (high_smob_value == SCM_BOOL_F) {
    high_smob_value = gpio_level_smob(HIGH);
  }
  return high_smob_value;
}

SCM
low_smob(void) {
  if (low_smob_value == SCM_BOOL_F) {
     low_smob_value = gpio_level_smob(LOW);
   }
  return low_smob_value;
}

void
init_gpio_level_type(void) {
  gpio_level_tag = scm_make_smob_type("gpio-level", sizeof(struct gpio_level));
  scm_set_smob_print(gpio_level_tag, print_gpio_level);
  scm_set_smob_free(gpio_level_tag, free_gpio_level);
  scm_set_smob_equalp(gpio_level_tag, gpio_level_equalp);
}

int
get_level_smob_value(SCM *level_smob, int *level) {
  struct gpio_level *gpio_level;
  scm_assert_smob_type(gpio_level_tag, *level_smob);
  gpio_level = (struct gpio_level *) SCM_SMOB_DATA (*level_smob);
  *level = gpio_level->bbio_level;
  return 0;
}
