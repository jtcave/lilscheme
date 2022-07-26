(define (factorial n)
  (define (iter x acc)
    (if (= x 0)
	acc
	(iter (- x 1) (* acc x))))
  (iter n 1))

(display (factorial 5))
(display (factorial 6))
