; This program is a guess the number game.
; The program takes the value of the DIP switch at start up,
; then shows 'b' if the DIP value is greater than the number,
; or 'S(5)' if the DIP value is less than the number.
; If the value is the same, it shows "C".
; This program demonstrates how comparisons can be implemented.

rst

; record the secret number
mov g b

; create a right shift lookup table
; loop start
ldi y 1 ; [lookup] 0x2
mov a x
mov a l
mov m x
mov x s
ldi y 0xF
mov c d
jni 0x2 ; jni [lookup]
; loop end

; store the secret number
mov x g ; x = secret
mov a x ; a = secret
mov a l ; a = secret << 4
mov a m ; a = secret & 0xF
mov y a ; y = secret & 0xF
ldi a 0x1
mov m y ; m[1] = secret & 0xF
mov a d ; a = secret & 0xF0
mov c m ; c = secret >> 4
ldi a 0x2
mov m c ; m[2] = secret >> 4

; create jump addresses
; m[5] = [lower]
ldi a 0x4
mov x l
ldi y 0x8
ldi a 0x5
mov m s
; m[6] = [less]
ldi a 0x5
mov x l
ldi y 0xB
ldi a 0x6
mov m s
; m[7] = [greater]
ldi a 0x5
mov x l
ldi y 0x8
ldi a 0x7
mov m s
; m[8] = [correct]
ldi a 0x5
mov x l
ldi y 0xE
ldi a 0x8
mov m s
; m[9] = [compare]
ldi a 0x2
mov x l
ldi y 0xE
ldi a 0x9
mov m s

; [compare] 0x2E
; store the guess number
mov x b ; x = guess
mov a x ; a = guess
mov a l ; a = guess << 4
mov a m ; a = guess & 0xF
mov y a ; y = guess & 0xF
ldi a 0x3
mov m y ; m[3] = guess & 0xF
mov a d ; a = guess & 0xF0
mov c m ; c = guess >> 4
ldi a 0x4
mov m c ; m[4] = guess >> 4

; load the higher 4 bits
mov x c ; x = guess >> 4
ldi a 0x2
mov y m ; y = secret >> 4

; compare the higher 4 bits
mov x d ; x = (guess >> 4) - (secret >> 4)
mov c x ; c = (guess >> 4) - (secret >> 4)
ldi a 0x5
jez m ; jez [lower]
mov a x ; a = (guess >> 4) - (secret >> 4)
mov a l ; a = ((guess >> 4) - (secret >> 4)) << 4
mov y m ; y = ((guess >> 4) - (secret >> 4)) & 0xF
mov c d ; c = ((guess >> 4) - (secret >> 4)) & 0xF0
ldi a 0x7
jez m ; jez [greater]
ldi a 0x6
jnz m ; jnz [less]

; load the lower 4 bits
; [lower] 0x48
ldi a 0x1
mov y m ; y = secret & 0xF
ldi a 0x3
mov x m ; x = guess & 0xF

; compare the lower 4 bits
mov x d ; x = (guess & 0xF) - (secret & 0xF)
mov c x ; c = (guess >> 4) - (secret >> 4)
ldi a 0x8
jez m ; jez [correct]
mov a x ; a = (guess & 0xF) - (secret & 0xF)
mov a l ; a = ((guess & 0xF) - (secret & 0xF)) << 4
mov y m ; y = ((guess & 0xF) - (secret & 0xF)) & 0xF
mov c d ; c = ((guess & 0xF) - (secret & 0xF)) & 0xF0
ldi a 0x7
jez m ; jez [greater]
ldi a 0x6
jnz m ; jnz [less]

; [greater] 0x58
ldi n 0xB
ldi a 0x9
jez m ; jez [compare]

; [less] 0x5B
ldi n 0x5
ldi a 0x9
jnz m ; jnz [compare]

; [correct] 0x5E
ldi n 0xC
ldi a 0x9
jez m ; jez [compare]
