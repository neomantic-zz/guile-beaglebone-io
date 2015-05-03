/*
Copyright (c) 2013 Adafruit

Original RPi.GPIO Author Ben Croston
Modified for BBIO Author Justin Cooper

This file incorporates work covered by the following copyright and
permission notice, all modified code adopts the original license:

Copyright (c) 2013 Ben Croston

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <pthread.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "event_gpio.h"
#include "common.h"

const char *stredge[4] = {"none", "rising", "falling", "both"};

// file descriptors
struct fdx
{
    int fd;
    unsigned int gpio;
    int initial;
    unsigned int is_evented;
    struct fdx *next;
};
struct fdx *fd_list = NULL;

// event callbacks
struct callback
{
    unsigned int gpio;
    void (*func)(unsigned int gpio);
    struct callback *next;
};
struct callback *callbacks = NULL;

// gpio exports
struct gpio_exp
{
    unsigned int gpio;
    struct gpio_exp *next;
};
struct gpio_exp *exported_gpios = NULL;

pthread_t threads;
int event_occurred[120] = { 0 };
int thread_running = 0;
int epfd = -1;

int gpio_export(unsigned int gpio)
{
    int fd, len;
    char str_gpio[10];

    if ((fd = open("/sys/class/gpio/export", O_WRONLY)) < 0)
    {
        return -1;
    }
    len = snprintf(str_gpio, sizeof(str_gpio), "%d", gpio);
    write(fd, str_gpio, len);
    close(fd);
    return 0;
}

void close_value_fd(unsigned int gpio)
{
    struct fdx *f = fd_list;
    struct fdx *temp;
    struct fdx *prev = NULL;

    while (f != NULL)
    {
        if (f->gpio == gpio)
        {
            close(f->fd);
            if (prev == NULL)
                fd_list = f->next;
            else
                prev->next = f->next;
            temp = f;
            f = f->next;
            free(temp);
        } else {
            prev = f;
            f = f->next;
        }
    }
}

int fd_lookup(unsigned int gpio)
{
    struct fdx *f = fd_list;
    while (f != NULL)
    {
        if (f->gpio == gpio)
            return f->fd;
        f = f->next;
    }
    return 0;
}

int open_value_file(unsigned int gpio)
{
    int fd;
    char filename[40];

    // create file descriptor of value file
    snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/value", gpio);
    if ((fd = open(filename, O_RDONLY | O_NONBLOCK)) < 0)
        return -1;
    return fd;
}

int gpio_unexport(unsigned int gpio)
{
    int fd, len;
    char str_gpio[10];

    if ((fd = open("/sys/class/gpio/unexport", O_WRONLY)) < 0)
        return -1;

    len = snprintf(str_gpio, sizeof(str_gpio), "%d", gpio);
    write(fd, str_gpio, len);
    close(fd);

    return 0;
}

int gpio_set_direction(unsigned int gpio, unsigned int in_flag)
{
        int fd;
        char filename[40];
        char direction[10] = { 0 };

        snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/direction", gpio);
        if ((fd = open(filename, O_WRONLY)) < 0)
            return -1;

        if (in_flag) {
            strncpy(direction, "out", ARRAY_SIZE(direction) - 1);
        } else {
            strncpy(direction, "in", ARRAY_SIZE(direction) - 1);
        }

        write(fd, direction, strlen(direction));
        close(fd);
        return 0;
}

int gpio_get_direction(unsigned int gpio, unsigned int *value)
{
    int fd;
    char direction[4] = { 0 };
    char filename[40];

    snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/direction", gpio);
    if ((fd = open(filename, O_RDONLY | O_NONBLOCK)) < 0)
        return -1;

    lseek(fd, 0, SEEK_SET);
    read(fd, &direction, sizeof(direction) - 1);

    if (strcmp(direction, "out") == 0) {
        *value = OUTPUT;
    } else {
        *value = INPUT;
    }

    return 0;
}

int gpio_set_value(unsigned int gpio, unsigned int value)
{
    int fd;
    char filename[40];
    char vstr[10];

    snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/value", gpio);

    if ((fd = open(filename, O_WRONLY)) < 0)
        return -1;

    if (value) {
        strncpy(vstr, "1", ARRAY_SIZE(vstr) - 1);
    } else {
        strncpy(vstr, "0", ARRAY_SIZE(vstr) - 1);
    }

    write(fd, vstr, strlen(vstr));
    close(fd);
    return 0;
}

int gpio_get_value(unsigned int gpio, unsigned int *value)
{
    int fd = fd_lookup(gpio);
    char ch;

    if (!fd)
    {
        if ((fd = open_value_file(gpio)) == -1)
            return -1;
    }

    lseek(fd, 0, SEEK_SET);
    read(fd, &ch, sizeof(ch));

    if (ch != '0') {
        *value = 1;
    } else {
        *value = 0;
    }

    return 0;
}

int gpio_set_edge(unsigned int gpio, unsigned int edge)
{
        int fd;
        char filename[40];

        snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/edge", gpio);

        if ((fd = open(filename, O_WRONLY)) < 0)
        return -1;

        write(fd, stredge[edge], strlen(stredge[edge]) + 1);
        close(fd);
        return 0;
}

int gpio_get_edge(unsigned int gpio, unsigned int *value)
{
  int fd;
  char filename[40];
  char ch;

  snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/edge", gpio);
  if ((fd = open(filename, O_RDONLY | O_NONBLOCK)) < 0)
    return -1;

  lseek(fd, 0, SEEK_SET);
  read(fd, &ch, sizeof(ch));

  if (ch == 'r') {
    *value = RISING;
  } else if (ch == 'f') {
    *value = FALLING;
  } else if (ch == 'b') {
    *value = BOTH;
  } else {
    *value = NONE;
  }

  close(fd);
  return 0;
}


unsigned int gpio_lookup(int fd)
{
    struct fdx *f = fd_list;
    while (f != NULL)
    {
        if (f->fd == fd)
            return f->gpio;
        f = f->next;
    }
    return 0;
}

int add_edge_callback(unsigned int gpio, void (*func)(unsigned int gpio))
{
    struct callback *cb = callbacks;
    struct callback *new_cb;

    new_cb = malloc(sizeof(struct callback));
    if (new_cb == 0)
        return -1;  // out of memory

    new_cb->gpio = gpio;
    new_cb->func = func;
    new_cb->next = NULL;

    if (callbacks == NULL) {
        // start new list
        callbacks = new_cb;
    } else {
        // add to end of list
        while (cb->next != NULL)
            cb = cb->next;
        cb->next = new_cb;
    }
    return 0;
}

void run_callbacks(unsigned int gpio)
{
    struct callback *cb = callbacks;
    while (cb != NULL)
    {
        if (cb->gpio == gpio)
            cb->func(cb->gpio);
        cb = cb->next;
    }
}

void remove_callbacks(unsigned int gpio)
{
    struct callback *cb = callbacks;
    struct callback *temp;
    struct callback *prev = NULL;

    while (cb != NULL)
    {
        if (cb->gpio == gpio)
        {
            if (prev == NULL)
                callbacks = cb->next;
            else
                prev->next = cb->next;
            temp = cb;
            cb = cb->next;
            free(temp);
        } else {
            prev = cb;
            cb = cb->next;
        }
    }
}

void set_initial_false(unsigned int gpio)
{
    struct fdx *f = fd_list;

    while (f != NULL)
    {
        if (f->gpio == gpio)
            f->initial = 0;
        f = f->next;
    }
}

int gpio_initial(unsigned int gpio)
{
    struct fdx *f = fd_list;

    while (f != NULL)
    {
        if ((f->gpio == gpio) && f->initial)
            return 1;
        f = f->next;
    }
    return 0;
}

typedef int (*thread_poller) (int *fd, int *epfd);

int *poll_thread(int *fd, int *epfd)
{
    struct epoll_event events;
    char buf;
    int n, i;

    // epoll for event
    for (i = 0; i<2; i++) // first time triggers with current state, so ignore
      if ((n = epoll_wait(*epfd, &events, 1, -1)) == -1)
	{
	  return -1;
	}

    if (n > 0) {
      lseek(events.data.fd, 0, SEEK_SET);
      if (read(events.data.fd, &buf, sizeof(char)) != 1)
	  return -2;
      if (events.data.fd != *fd)
	return -3;
    }

    close(*epfd);
    return 0;
}

int gpio_is_evented(unsigned int gpio)
{
    struct fdx *f = fd_list;
    while (f != NULL)
    {
        if (f->gpio == gpio)
            return 1;
        f = f->next;
    }
    return 0;
}

int gpio_event_add(unsigned int gpio)
{
    struct fdx *f = fd_list;
    while (f != NULL)
    {
        if (f->gpio == gpio)
        {
            if (f->is_evented)
                return 1;

            f->is_evented = 1;
            return 0;
        }
        f = f->next;
    }
    return 0;
}

int gpio_event_remove(unsigned int gpio)
{
    struct fdx *f = fd_list;
    while (f != NULL)
    {
        if (f->gpio == gpio)
        {
            f->is_evented = 0;
            return 0;
        }
        f = f->next;
    }
    return 0;
}

int blocking_wait_for_edge(unsigned int gpio, unsigned int *value)
{
  int fd;
  char buf, filename[40];
  struct epoll_event ev;
  int epfd, n, i;

  if ((epfd = epoll_create(1)) == -1)
    return 1;

  snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/value", gpio);
  if ((fd = open(filename, O_RDONLY | O_NONBLOCK)) < 0)
    return -1;

  // add to epoll fd
  ev.events = EPOLLIN | EPOLLET | EPOLLPRI;
  ev.data.fd = fd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    return 2;

  // epoll for event
  for (i = 0; i<2; i++) // first time triggers with current state, so ignore
    if ((n = epoll_wait(epfd, &ev, 1, -1)) == -1)
      {
	return 5;
      }

  if (n > 0)
    {
      lseek(ev.data.fd, 0, SEEK_SET);
      if (read(ev.data.fd, &buf, sizeof(char)) != 1)
        {
	  return 6;
        }
      if (ev.data.fd != fd)
        {
	  return 7;
        }
    }

  if (buf != '0') {
    *value = LOW;
  } else {
    *value = HIGH;
  }

  close(epfd);
  return 0;
}

typedef int (*thread_with_callbacks) (thread_poller poller);
typedef thread_poller struct {
  int *fd_arg;
  int *epfd_arg;
  thread_with_callbacks runner;
};
int add_edge_detect(unsigned int gpio, thread_with_callbacks runner)
// return values:
// 0 - Success
// 1 - Edge detection already added
// 2 - Other error
{
  int fd, epfd;
  struct epoll_event ev;

  if ((fd = open_value_file(gpio)) == -1)
    return 2;

  // create epfd if not already open
  if ((epfd = epoll_create(1)) == -1)
    return 2;

  // add to epoll fd
  ev.events = EPOLLIN | EPOLLET | EPOLLPRI;
  ev.data.fd = fd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    return 2;
  thread_poller *poller = malloc(sizeof(thread_poller));
  if (poller == NULL)
    return -1;

  poller.runner = runner;
  poller.fd_arg = &fd;
  poller.epfd_arg = &epfd;
  runner(thread_poller);
  return 0;
}

void remove_edge_detect(unsigned int gpio)
{
    struct epoll_event ev;
    int fd = fd_lookup(gpio);

    // delete callbacks for gpio
    remove_callbacks(gpio);

    // delete epoll of fd
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);

    // set edge to none
    gpio_set_edge(gpio, NONE);

    // unexport gpio
    gpio_event_remove(gpio);

    // clear detected flag
    event_occurred[gpio] = 0;
}

int event_detected(unsigned int gpio)
{
    if (event_occurred[gpio]) {
        event_occurred[gpio] = 0;
        return 1;
    } else {
        return 0;
    }
}

void event_cleanup(void)
{
    close(epfd);
    thread_running = 0;
}
