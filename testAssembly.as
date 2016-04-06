.entry LOOP
MAIN: inc K
;mov *,w
.extern W
	add r2, STR
LOOP: jmp W
	prn #-5
STR: .string "ab"
	sub r1,r4
	hlt
.entry STR
K:.data 2,7