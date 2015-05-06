#!/usr/local/bin/guile -s
!#
(add-to-load-path (string-append (dirname (dirname (current-filename)))))

(use-modules
 (ice-9 threads)
 (beagleio))

(define switch-count 0)

(define (callback_one value)
  (display value)
  (newline)
  (display "********* Callback 1\n"))

(define (callback_two value)
  (display value)
  (newline)
  (display "********* Callback 2\n"))

(define (switch-callback)
  (lambda (value)
    (set! switch-count (+ 1 switch-count))
    (display (string-append "Button Press: " (number->string switch-count)))
    (newline)))

(let ((gpio (gpio-setup "P8_12"))
      (thread '()))
  (dynamic-wind
    (lambda ()
      (display "setting up\n")
      (gpio-direction-set! gpio INPUT)
      (gpio-edge-set! gpio FALLING)
      (gpio-callback-append gpio callback_one)
      (gpio-callback-append gpio callback_two)
      (gpio-callback-append gpio (switch-callback)))
    (lambda ()
      (display "listening up\n")
      (set! thread (gpio-edge-make-thread gpio))
      (sleep 10))
    (lambda ()
      (display (string-append "The button was pressed " (number->string switch-count) " times."))
      (newline)
      (display "closing\n")
      (cancel-thread thread)
      (gpio-close gpio))))
