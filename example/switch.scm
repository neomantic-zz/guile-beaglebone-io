#!/usr/local/bin/guile -s
!#
(add-to-load-path (string-append (dirname (dirname (current-filename)))))

(use-modules
 (beagleio))

(define (callback_one gpio value)
  (display gpio)
  (newline)
  (display value)
  (newline)
  (display "First callback!!!\n"))

(define (callback_two gpio value)
  (display "Second callback!!!\n"))

(let ((gpio (gpio-setup "P8_12")))
  (dynamic-wind
    (lambda ()
      (display "setting up\n")
      (gpio-direction-set! gpio INPUT)
      (gpio-edge-set! gpio FALLING)
      (gpio-callback-append gpio callback_one)
      (gpio-callback-append gpio callback_two))
    (lambda ()
      (display "listening up\n")
      (gpio-edge-wait gpio))
    (lambda ()
      (display "closing\n")
      (gpio-close gpio))))
