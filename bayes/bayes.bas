10 ' BAYESIAN WORD CLASSIFIER DEMO - SANYO MBC-550
20 ' Text UI + Graphics Bigram Matrix Visualization
30 DEFINT A-Z
40 CONST NUMLET = 26
50 CONST CELW = 10
60 CONST CELH = 7
70 DIM BGENG(NUMLET, NUMLET)
80 DIM BGNON(NUMLET, NUMLET)
90 TOT_ENG = 0
100 TOT_NON = 0
110 GOSUB 5000   ' InitScreen
120 GOSUB 6000   ' TrainFromCorpus
130 ' Main Loop
140 GOSUB 7000
150 GOTO 130
4990 ' ------------------------------------------------------------
5000 SCREEN 0: COLOR 7,0: CLS
5010 PRINT "==============================================="
5020 PRINT "   BAYESIAN WORD CLASSIFIER - MBC-555 DEMO"
5030 PRINT "==============================================="
5040 PRINT
5050 PRINT "Training corpus: Preamble to the Constitution"
5060 PRINT "Bigram model:    26 x 26 matrix"
5070 PRINT "User feedback:   ENABLED"
5080 PRINT
5090 PRINT "Keys:"
5100 PRINT "  G - Graphics bigram matrix"
5110 PRINT "  S - Summary of model"
5120 PRINT "  Q - Quit"
5130 PRINT
5140 PRINT "Press any key to begin..."
5150 A$ = INPUT$(1)
5160 CLS
5170 RETURN
5990 ' ------------------------------------------------------------
6000 ' TrainFromCorpus
6010 RESTORE 6500
6020 DO
6030   READ W$
6040   IF W$ = "END" THEN EXIT DO
6050   GOSUB 9000   ' UpdateCountsEnglish
6060 LOOP
6070 RETURN
6500 DATA "WE","THE","PEOPLE","OF","THE","UNITED","STATES","IN","ORDER","TO","FORM","A","MORE","PERFECT","UNION"
6510 DATA "ESTABLISH","JUSTICE","INSURE","DOMESTIC","TRANQUILITY","PROVIDE","FOR","THE","COMMON","DEFENCE"
6520 DATA "PROMOTE","THE","GENERAL","WELFARE","AND","SECURE","THE","BLESSINGS","OF","LIBERTY","TO","OURSELVES"
6530 DATA "AND","OUR","POSTERITY","DO","ORDAIN","AND","ESTABLISH","THIS","CONSTITUTION","FOR","THE","UNITED","STATES","OF","AMERICA"
6540 DATA "END"
6990 ' ------------------------------------------------------------
7000 ' Text UI
7010 SCREEN 0: COLOR 7,0: CLS
7020 PRINT "----------------------------------------------"
7030 PRINT "BAYESIAN WORD CLASSIFIER - Text Mode"
7040 PRINT "----------------------------------------------"
7050 PRINT
7060 PRINT "Enter a word (OR G/S/Q): ";
7070 W$ = UCASE$(INPUT$(20))
7080 IF W$ = "G" THEN GOSUB 11000: RETURN
7090 IF W$ = "S" THEN GOSUB 12000: RETURN
7100 IF W$ = "Q" THEN END
7110 IF LEN(W$) < 2 THEN
7120   PRINT: PRINT "Word too short. Try again."
7130   SLEEP 1
7140   RETURN
7150 END IF
7160 GOSUB 8000   ' ClassifyWord
7170 PRINT
7180 PRINT "Model guess: "; GUESS$
7190 PRINT "Confidence (relative): "; INT(CONF * 100); "%"
7200 GOSUB 10000  ' Confidence bar
7210 PRINT
7220 PRINT "Was I right (Y/N)? ";
7230 A$ = UCASE$(INPUT$(1))
7240 IF A$ = "N" THEN
7250   PRINT: PRINT "Is the word English (Y/N)? ";
7260   B$ = UCASE$(INPUT$(1))
7270   IF B$ = "Y" THEN
7280     GOSUB 9000
7290   ELSE
7300     GOSUB 9500
7310   END IF
7320   PRINT: PRINT "Model updated."
7330   SLEEP 1
7340 END IF
7350 RETURN
7990 ' ------------------------------------------------------------
8000 ' ClassifyWord
8010 ENG_SCORE = 0#: NON_SCORE = 0#
8020 FOR I = 1 TO LEN(W$) - 1
8030   A = ASC(MID$(W$, I, 1)) - 64
8040   B = ASC(MID$(W$, I+1, 1)) - 64
8050   IF A>=1 AND A<=NUMLET AND B>=1 AND B<=NUMLET THEN
8060     ENG_SCORE = ENG_SCORE + LOG((BGENG(A,B)+1#)/(TOT_ENG+NUMLET*NUMLET))
8070     NON_SCORE = NON_SCORE + LOG((BGNON(A,B)+1#)/(TOT_NON+NUMLET*NUMLET))
8080   END IF
8090 NEXT
8100 IF ENG_SCORE > NON_SCORE THEN
8110   GUESS$ = "English"
8120 ELSE
8130   GUESS$ = "Not English"
8140 END IF
8150 IF ENG_SCORE > NON_SCORE THEN
8160   CONF = 1#/(1#+EXP(NON_SCORE-ENG_SCORE))
8170 ELSE
8180   CONF = 1#/(1#+EXP(ENG_SCORE-NON_SCORE))
8190 END IF
8200 RETURN
8990 ' ------------------------------------------------------------
9000 ' UpdateCountsEnglish
9010 W$ = UCASE$(W$)
9020 FOR I = 1 TO LEN(W$)-1
9030   A = ASC(MID$(W$,I,1)) - 64
9040   B = ASC(MID$(W$,I+1,1)) - 64
9050   IF A>=1 AND A<=NUMLET AND B>=1 AND B<=NUMLET THEN
9060     BGENG(A,B) = BGENG(A,B) + 1
9070     TOT_ENG = TOT_ENG + 1
9080   END IF
9090 NEXT
9100 RETURN
9500 ' UpdateCountsNonEnglish
9510 W$ = UCASE$(W$)
9520 FOR I = 1 TO LEN(W$)-1
9530   A = ASC(MID$(W$,I,1)) - 64
9540   B = ASC(MID$(W$,I+1,1)) - 64
9550   IF A>=1 AND A<=NUMLET AND B>=1 AND B<=NUMLET THEN
9560     BGNON(A,B) = BGNON(A,B) + 1
9570     TOT_NON = TOT_NON + 1
9580   END IF
9590 NEXT
9600 RETURN
9990 ' ------------------------------------------------------------
10000 ' Confidence Bar
10010 PRINT
10020 PRINT "English score bar: ";
10030 BARLEN = 20
10040 ENGBAR = INT(CONF * BARLEN)
10050 FOR I = 1 TO ENGBAR: PRINT "#";: NEXT
10060 FOR I = ENGBAR+1 TO BARLEN: PRINT "-";: NEXT
10070 PRINT
10080 PRINT "Non-English score bar: ";
10090 NONCONF = 1# - CONF
10100 NONBAR = INT(NONCONF * BARLEN)
10110 FOR I = 1 TO NONBAR: PRINT "#";: NEXT
10120 FOR I = NONBAR+1 TO BARLEN: PRINT "-";: NEXT
10130 PRINT
10140 RETURN
10990 ' ------------------------------------------------------------
11000 ' Graphics Mode
11010 SCREEN 1: CLS
11020 COLOR 3
11030 FOR A = 1 TO NUMLET
11040   FOR B = 1 TO NUMLET
11050     FREQ = BGENG(A,B) + BGNON(A,B)
11060     C = ComputeColor(FREQ)
11070     X = (A-1)*CELW
11080     Y = (B-1)*CELH
11090     LINE (X,Y)-(X+CELW-1,Y+CELH-1), C, BF
11100   NEXT
11110 NEXT
11120 SCREEN 0
11130 PRINT "Bigram frequency matrix (total counts)"
11140 PRINT "Color: Blue=LOW, Cyan=MED, White=HIGH"
11150 PRINT "Press any key to return..."
11160 A$ = INPUT$(1)
11170 SCREEN 0
11180 RETURN
11990 ' ------------------------------------------------------------
12000 ' Summary
12010 SCREEN 0: COLOR 7,0: CLS
12020 PRINT "=============================================="
12030 PRINT "Model status summary"
12040 PRINT "=============================================="
12050 PRINT
12060 PRINT "Total English bigrams:     "; TOT_ENG
12070 PRINT "Total non-English bigrams: "; TOT_NON
12080 PRINT
12090 PRINT "Press any key to return..."
12100 A$ = INPUT$(1)
12110 RETURN
12990 ' ------------------------------------------------------------
13000 FUNCTION ComputeColor(FREQ)
13010 IF FREQ < 3 THEN ComputeColor = 1: EXIT FUNCTION
13020 IF FREQ < 10 THEN ComputeColor = 3: EXIT FUNCTION
13030 ComputeColor = 7
13040 END FUNCTION
