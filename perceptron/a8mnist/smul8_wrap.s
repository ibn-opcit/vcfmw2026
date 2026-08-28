.importzp multiplicand, multiplier, result_lo, result_hi
.import   SMUL8
.export   _smul8

_smul8:
    ; A = b (low byte of int arg), X = a (high byte of int arg)
    ; No software stack used at all
    STA multiplier
    STX multiplicand

    JSR SMUL8

    LDA result_lo
    LDX result_hi
    RTS