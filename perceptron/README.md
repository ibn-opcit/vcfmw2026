# Runbook (onsite)

Runs on the Atari 800XL. 

## From power down

- Ensure the Koalapad (the one labeled `ATARI`) is plugged into PORT 1.
- Ensure the S-Drive Max (little white box) is plugged into the base machine and has power. It should have a pre-loaded micro-SD card in it already.
- Power on (left rear)
- Boots to DOS

### Notes and troubleshooting

- The S-Drive should have the following mapping:
    - D1: A8SPARTA
    - D2: A8DAT
    - D3: A8MNIST
- Reconstruct if necessary using on-screen UI. There should be a stylus floating around for dealing with the tiny touchscreen.

## From DOS

- `D3:A8MNIST`


### Notes and troubleshooting

- Left side of screen shows a (binarized) 28 x 28 digit image (either from the MNIST test set or drawn by the user)
- Right side shows a crude confidence meter for the inference 0-9 (left to right)
- Default behavior is "demo mode," where the machine picks a random image from the collection on D2
- Interrupt this at any time by pressing a key that is not `C` or `D` (modulo one evaluation). You are now in drawing mode.
- Hold down the left tablet button while sketching. Inference commences when the button is released.
- Following an inference, resume demo mode with `D`, or press any other key to draw another digit
- You can recalibrate the pad by pressing `C` (not generally needed)


## Making changes

- The development environment requires [cc65](https://cc65.github.io/) and the [Altirra emulator](https://www.virtualdub.org/altirra.html)
    - Configure the emulator with the built-in `XL/XE Computer` profile
    - Configure three virtual floppy drives
        - D1: A DOS disk (I used SpartaDOS 3.2d)
        - D2: A blank data disk. This one you will populate (one-time) with test images. I had trouble with the double-sided options but was able to get one to work that holds ~130 images.
        - D3: A blank disk that will hold your executable, `A8MNIST.XEX`
    - A "deployment" consists of refreshing the contents of these images as necessary and copying them to the micro-SD card read by the SDrive Max (this has to be done by sneakernet; the USB port on the SDrive is for power only).
        - You can drag-and-drop files from Windows into the Explore Disk window in Altirra
    - To build, make sure `cc65` is on your path and run `./build.sh` from the `a8mnist` directory
    - Outputs `a8mnist.xex` executable
    - Run in emulator and/or deploy. If deploying, rename to `A8MNIST.COM`