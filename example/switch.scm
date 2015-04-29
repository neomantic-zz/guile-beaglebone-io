#!/usr/local/bin/guile -s
!#
(add-to-load-path (string-append (dirname (dirname (current-filename)))))

(use-modules
 (beagleio))

(define (callback_one value)
  (display "First callback!!!\n"))

(define (callback_two value)
  (display "Second callback!!!\n"))

(let ((gpio (gpio-setup "P8_12")))
  (dynamic-wind
    (lambda ()
      (display "setting up\n")
      (gpio-direction-set! gpio INPUT)
      (gpio-edge-set! gpio RISING)
      (gpio-callback-append gpio callback_one)
      (gpio-callback-append gpio callback_two))
    (lambda ()
      (display "listening up\n")
      (gpio-event-wait gpio))
    (lambda ()
      (display "closeing\n")
      (gpio-close gpio))))
