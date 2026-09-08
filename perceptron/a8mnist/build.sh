#!/bin/sh

cl65 -t atari -Ln a8mnist.lbl -C atari.cfg -Wl -m,mapfile.txt -o a8mnist.xex \
    main.c infer.c koala.c smul8.s smul8_wrap.s model.s
