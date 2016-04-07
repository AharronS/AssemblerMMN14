.entry LOOP
MAIN: inc K
mov *, W




.extern W
	add r2, STR
LOOP: jmp W
	prn #-5
STR: .string "ab"
	sub r1,r4
	stop
.entry STR
K:.data 2,7