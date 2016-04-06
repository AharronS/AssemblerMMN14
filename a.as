;file a.as includes routine PRTSTR. This routine prints a string              

            .entry PRTSTR 
PRTSTR:     mov STRADD, r2 
LOOP:       cmp  #0, r2
            jmp BYE 
            prn r2  
            inc r2 
            bne LOOP 
BYE:        rts   
            .extern STRADD 
		  
				
