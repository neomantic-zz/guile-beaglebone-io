#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "event_gpio.h"
#include "guile_beagleio_gpio.h"
#include "scm_gpio_type.h"
#include "scm_gpio_setting_type.h"
#include "scm_gpio_value_type.h"
#include "scm_gpio_direction_type.h"
#include "scm_gpio_edge_type.h"
#include <sys/epoll.h>
#include <fcntl.h>

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
  success = gpio->getDirection(gpio, &direction);
  if (success == 0)
    return scm_new_gpio_direction_smob(direction);

  if (success != -1)
    return scm_gpio_throw("unable to write /sys/class/gpio/*/direction to reset it");
  return scm_gpio_throw("unable to read /sys/class/gpio/*/direction");
}

SCM
set_value(SCM gpio_smob, SCM gpio_value_smob)
{
  Gpio *gpio;
  GpioValue *gpio_value;
  int success;
  scm_assert_gpio_smob_type(&gpio_smob);
  scm_assert_gpio_value_smob(&gpio_value_smob);
  gpio = (Gpio *) SCM_SMOB_DATA(gpio_smob);
  gpio_value = (GpioValue *)SCM_SMOB_DATA(gpio_value_smob);
  success = gpio->setValue(gpio, gpio_value->bbio_value);

  if (success == 0)
    return gpio_smob;

  if (success == -2)
    return scm_gpio_throw("The gpio channel has not been setup as output");
  return scm_gpio_throw("unable to read or write to /sys/class/gpio/*/direction");

}

SCM
get_value(SCM gpio_smob)
{
  Gpio *gpio;
  unsigned int current_value;

  scm_assert_gpio_smob_type(&gpio_smob);
  gpio = (Gpio *) SCM_SMOB_DATA (gpio_smob);
  if(gpio->getValue(gpio, &current_value) == -1)
    return scm_gpio_throw("unable to read /sys/class/gpio/*/value");

  return scm_new_gpio_value_smob(current_value);
}

SCM
set_edge(SCM gpio_smob,  SCM gpio_edge_smob)
{
  Gpio *gpio;
  GpioEdge *gpio_edge;
  int success;

  scm_assert_gpio_smob_type(&gpio_smob);
  scm_assert_gpio_edge_smob(&gpio_edge_smob);

  gpio = (Gpio *) SCM_SMOB_DATA(gpio_smob);
  gpio_edge = (GpioEdge *) SCM_SMOB_DATA(gpio_edge_smob);

  success = gpio->setEdge(gpio, gpio_edge->bbio_value);

  if (success == 0)
    return gpio_smob;

  if (success == -2)
    return scm_gpio_throw("The gpio export must be set to INPUT");
  return scm_gpio_throw("unable to write to /sys/class/gpio/*/value");
}

SCM
append_callback(SCM gpio_smob, SCM procedure)
{
  Gpio *gpio;
  int success;

  scm_assert_gpio_smob_type(&gpio_smob);

  gpio = (Gpio *) SCM_SMOB_DATA(gpio_smob);
  success = gpio->appendEventCallback(gpio, procedure);

  /* TODO:  Make this pretty*/
  if (success == -1)
      return scm_gpio_throw("unable to read to /sys/class/gpio/*/direction");

  if (success == -2 )
    return scm_gpio_throw("The gpio export must be set to INPUT for edge detection");

  if (success == -3)
    return scm_gpio_throw("Add event detection using gpio-edge-set first before adding a callback");

  return gpio_smob;
}

static SCM
detecting(void *data)
{
  Gpio *gpio;
  struct epoll_event event;
  char buf, filename[40];
  int n, i, epfd, fd;
  struct scm_callback *current_scm_callback;
  struct timeval tv_timenow;
  unsigned long long timenow;
  SCM gpio_value_smob, return_value;
  int woot = 0;

  gpio = (Gpio*)data;
  return_value = SCM_BOOL_F;
  gpio_value_smob = scm_new_gpio_value_smob(LOW);

  if ((epfd = epoll_create(1)) == -1)
    return scm_gpio_throw("what 1");

  snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/value", gpio->pin_number);
  if ((fd = open(filename, O_RDONLY | O_NONBLOCK)) < 0)
    return scm_gpio_throw("Unable to read value file");

  // add to epoll fd
  event.events = EPOLLIN | EPOLLET | EPOLLPRI;
  event.data.fd = fd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) == -1)
    return scm_gpio_throw("Unable to detect");

  // epoll for event
  for (i = 0; i<2; i++) // first time triggers with current state, so ignore
    if ((n = epoll_wait(epfd, &event, 1, -1)) == -1) {
      woot = 0;
      /* TODO:  raise?*/
      //return SCM_BOOL_F;
    }

  if (n > 0) {
    lseek(event.data.fd, 0, SEEK_SET);
    if (read(event.data.fd, &buf, sizeof(char)) != 1) {
  	/* TODO:  raise?*/
      woot = 0;
      //return SCM_BOOL_F;
    }

    current_scm_callback = gpio->scm_gpio_callbacks;
    while (current_scm_callback != NULL) {
      gettimeofday(&tv_timenow, NULL);
      timenow = tv_timenow.tv_sec*1E6 + tv_timenow.tv_usec;
      if (current_scm_callback->bouncetime == 0 ||
    	  timenow - current_scm_callback->lastcall > current_scm_callback->bouncetime*1000 ||
    	  current_scm_callback->lastcall == 0 ||
    	  current_scm_callback->lastcall > timenow) {

    	// save lastcall before calling func to prevent reentrant bounce
    	current_scm_callback->lastcall = timenow;
    	return_value = scm_call_1(current_scm_callback->procedure, gpio_value_smob);
      }
      current_scm_callback = current_scm_callback->next;
    }
  }

  return return_value;
}

static SCM
catch_handler(void *data, SCM key, SCM args)
{
  return SCM_BOOL_T;
}

SCM
listen_for_edge(SCM gpio_smob) {
  Gpio *gpio;
  SCM detectable, returned;
  scm_assert_gpio_smob_type(&gpio_smob);
  gpio = (Gpio *) SCM_SMOB_DATA(gpio_smob);
  printf("CHAD wtf1\n");

  detectable = gpio->edgeDetectable(gpio);
  if (!scm_is_true(detectable));
    return detectable;

  SCM thread;
  printf("CHAD wtf2\n");
  thread = scm_spawn_thread(detecting, gpio, catch_handler, 0);
  printf("CHAD wtf9\n");
  return scm_join_thread(thread);
}

SCM
wait_for_edge(SCM gpio_smob)
{
  Gpio *gpio;
  int success;
  unsigned int value;
  struct scm_callback *current_scm_callback;
  SCM detectable, gpio_value_smob, return_value;

  scm_assert_gpio_smob_type(&gpio_smob);
  gpio = (Gpio *) SCM_SMOB_DATA(gpio_smob);

  detectable = gpio->edgeDetectable(gpio);
  if (detectable != SCM_BOOL_T)
    return detectable;

  gpio_value_smob = scm_new_gpio_value_smob(LOW);
  return_value = SCM_BOOL_F;

  success = blocking_wait_for_edge((unsigned int) gpio->pin_number, &value);
  if (success < 0) {
    return scm_gpio_throw("Unable to read value");
  }
  else if (success > 0) {
    return scm_gpio_throw("An error occurred when waiting for an event");
  }

  if (value == HIGH)
    gpio_value_smob = scm_new_gpio_value_smob(HIGH);

  current_scm_callback = gpio->scm_gpio_callbacks;
  while (current_scm_callback != NULL)
    {
      return_value = scm_call_1(current_scm_callback->procedure, gpio_value_smob);
      current_scm_callback = current_scm_callback->next;
    }
  return return_value;
}

SCM
close_channel(SCM gpio_smob)
{
  Gpio *gpio;
  int result;
  scm_assert_gpio_smob_type(&gpio_smob);
  gpio = (Gpio *) SCM_SMOB_DATA (gpio_smob);
  result = gpio->close(gpio);
  if (result == -1)
    return scm_gpio_throw("unable to write /sys/class/gpio/*/unexport");
  return gpio_smob;
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
  init_gpio_edge_type();
  scm_c_define_gsubr("gpio-setup", 1, 0, 0, setup_channel);
  scm_c_define_gsubr("gpio-close", 1, 0, 0, close_channel);
  scm_c_define_gsubr("gpio-direction-set!", 2, 0, 0, set_direction);
  scm_c_define_gsubr("gpio-direction", 1, 0, 0, get_direction);
  scm_c_define_gsubr("gpio-edge-set!", 2, 0, 0, set_edge);

  scm_c_define_gsubr("gpio-number-lookup", 1, 0, 0, lookup_gpio_number);
  scm_c_define_gsubr("gpio?", 1, 0, 0, scm_gpio_type_p);
  scm_c_define("INPUT", scm_new_gpio_direction_smob(INPUT));
  scm_c_define("OUTPUT", scm_new_gpio_direction_smob(OUTPUT));
  scm_c_define_gsubr("gpio-value-set!", 2, 0, 0, set_value);
  scm_c_define_gsubr("gpio-value", 1, 0, 0, get_value);
  scm_c_define("HIGH", scm_new_gpio_value_smob(HIGH));
  scm_c_define("LOW", scm_new_gpio_value_smob(LOW));

  scm_c_define("NONE", scm_new_gpio_edge_smob(NONE));
  scm_c_define("RISING", scm_new_gpio_edge_smob(RISING));
  scm_c_define("FALLING", scm_new_gpio_edge_smob(FALLING));
  scm_c_define("BOTH", scm_new_gpio_edge_smob(BOTH));
  scm_c_define_gsubr("gpio-edge-set!", 2, 0, 0, set_edge);
  scm_c_define_gsubr("gpio-callback-append", 2, 0, 0, append_callback);
  scm_c_define_gsubr("gpio-edge-wait", 1, 0, 0, wait_for_edge);
  scm_c_define_gsubr("gpio-edge-event-thread", 1, 0, 0, listen_for_edge);

  initialized = 1;
}
