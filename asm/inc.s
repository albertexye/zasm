rst     ; reset
ldi c 1 ; set conditional
ldi y 1 ; add one at a time
mov n x ; show x
mov x s ; increment x
jni 3   ; infinite loop
