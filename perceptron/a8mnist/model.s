.segment "DATA"

.export _model_weights
.export _model_bias

_model_weights:
    .incbin "MNISTQ.BIN"

_model_bias:
    .incbin "MNISTB.BIN"
