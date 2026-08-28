# Runbook (onsite)

## From power down

- Insert USB stick in Sanyo Gotek (stick is labeled "SANYO")
- Power on
- Twiddle Gotek knob to make sure correct disk image is selected ("BAYES")
- Press reset (left side of keyboard).  System boots to DOS.

### Notes and troubleshooting

- _No video_
    - Ensure you're plugged in to the Dell monitor labeled "SANYO" on the back. The MBC-555 requires a monitor that can deal with a 19 kHz RGB signal.

## From DOS prompt

- `BASIC`
- `load "BAYES.BAS"`
- `RUN`
- Instructions on-screen

### Notes and troubleshooting

- This demo trains on startup and has no model persistence. Learnings are lost when execution stops.
- There are no negative examples (non-English bigrams) in the training data. Predictions will warm up after a few inputs.

## Making changes

- Edit code directly on the machine (I don't have a working emulator for this)
- Save your changes, e.g. `save "BAYES2.BAS"`
- You can extract the saved file from the USB stick on a modern machine using a tool such as [WinImage](https://www.winimage.com/download.htm)