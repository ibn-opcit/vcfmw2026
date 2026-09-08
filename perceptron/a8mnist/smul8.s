.segment "ZEROPAGE"
multiplicand: .res 1
multiplier:   .res 1
result_lo:    .res 1
result_hi:    .res 1
sign_temp:    .res 1

.export multiplicand, multiplier, result_lo, result_hi, sign_temp

.code
.export SMUL8

SMUL8:
    TXA
    PHA

    ; Result sign is just the sign of the signed argument (multiplicand)
    LDA multiplicand
    AND #$80
    STA sign_temp

    ; Absolutize multiplicand only
    LDA multiplicand
    BPL @pos_a
    EOR #$FF
    CLC
    ADC #$01
@pos_a:
    STA multiplicand

    ; multiplier is unsigned, use as-is
    LDA #$00
    STA result_lo
    LDX #8

@loop:
    LSR multiplier
    BCC @no_add
    CLC
    ADC multiplicand
@no_add:
    ROR A
    ROR result_lo
    DEX
    BNE @loop

    STA result_hi

    LDA sign_temp
    BPL @done

    LDA result_lo
    EOR #$FF
    CLC
    ADC #$01
    STA result_lo
    LDA result_hi
    EOR #$FF
    ADC #$00
    STA result_hi

@done:
    PLA
    TAX
    RTS