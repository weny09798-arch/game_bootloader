        AREA |.text|, CODE, READONLY
        THUMB
        EXPORT board_jump_asm
board_jump_asm PROC
        MOVS r2, #0
        MSR CONTROL, r2
        MSR BASEPRI, r2
        MSR FAULTMASK, r2
        ISB
        MSR MSP, r0
        BX r1
        ENDP
        END
