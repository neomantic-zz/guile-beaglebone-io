#include "scm_gpio_edge_type.h"
#include <libguile.h>
#include "scm_gpio_setting_type.h"

static scm_t_bits gpio_edge_tag;

void
scm_assert_gpio_edge_smob(SCM *smob)
{
  scm_assert_smob_type(gpio_edge_tag, *smob);
}

static int
scm_gpio_edge_print(SCM gpio_edge_smob, SCM port, scm_print_state *pstate)
{
  GpioEdge *gpio_edge;
  scm_assert_gpio_edge_smob(&gpio_edge_smob);
  gpio_edge = (GpioEdge*)SCM_SMOB_DATA(gpio_edge_smob);
  scm_puts("#<gpio-edge level: ", port);
  if (gpio_edge->bbio_value == HIGH) {
    scm_puts("high", port);
  } else {
    scm_puts("low", port);
  }
  scm_puts(">", port);
  return 1;
}

static size_t
scm_gpio_edge_free(SCM gpio_edge_smob)
{
  GpioEdge *gpio_edge;
  scm_assert_gpio_edge_smob(&gpio_edge_smob);
  gpio_edge = (GpioEdge*) SCM_SMOB_DATA(gpio_edge_smob);
  scm_gc_free(gpio_edge, sizeof(GpioEdge), "gpio-edge");
  return 0;
}

static SCM
scm_gpio_edge_equalp(SCM gpio_edge_smob, SCM other_gpio_edge_smob)
{
  GpioEdge *gpio_edge, *other_gpio_edge;
  scm_assert_gpio_edge_smob(&gpio_edge_smob);
  scm_assert_gpio_edge_smob(&other_gpio_edge_smob);
  gpio_edge = (GpioEdge*) SCM_SMOB_DATA(gpio_edge_smob);
  other_gpio_edge = (GpioEdge*) SCM_SMOB_DATA(other_gpio_edge_smob);
  if (gpio_edge->bbio_value == other_gpio_edge->bbio_value)
    return SCM_BOOL_T;
  return SCM_BOOL_F;
}

SCM
scm_new_gpio_edge_smob(const int value)
{
  GpioEdge *gpio_edge;
  gpio_edge = (GpioEdge*) scm_gc_malloc(sizeof(GpioEdge), "gpio-edge");
  gpio_edge->update_func = SCM_BOOL_F;
  gpio_edge->bbio_value = value;
  return scm_new_smob(gpio_edge_tag, (scm_t_bits) gpio_edge);
}

void
init_gpio_edge_type(void)
{
  gpio_edge_tag = scm_make_smob_type("gpio-edge", sizeof(GpioEdge));
  scm_set_smob_print(gpio_edge_tag, scm_gpio_edge_print);
  scm_set_smob_free(gpio_edge_tag, scm_gpio_edge_free);
  scm_set_smob_equalp(gpio_edge_tag, scm_gpio_edge_equalp);
}
