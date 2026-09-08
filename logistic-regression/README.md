# Runbook (onsite)

Runs on the TRS-80 4P. `LR.BAS` in the repo is a raw extract from a TRS-80 disk image and must be edited in an emulator or on the machine (see below). 

## From power down

- Make sure SD card is in hard drive (front right)
- Power on (red switch on front panel)
- Boots to LS-DOS

### Notes and troubleshooting

- If display is janky, let the machine warm up a minute


## From LS-DOS

- A copy of the demo is on the machine's hard drive. 
- Press `2` to select drive partition 2
- Select `GBASIC` and press enter to run. No parameters.
- `load "2:LR/BAS"`
- `RUN`


### Notes and troubleshooting

- Left side of screen shows a sampling of the Titanic passengers in two dimensions (age vertical, gender horizontal). Dots are survivors and circles are casualties. Male is on the right. 
- Right side of screen shows loss vs. time as gradient descent does its thing
- Bottom line of screen shows current iteration, loss, and current values of the 4 model parameters
- Left side will also show the projection of the current separation plane onto those two features (age and gender). Usually this doesn't show up until late in the fit when the model is almost converged.


## Making changes

- You can edit code directly on the machine
    - To persist changes off the TRS-80, save the edited source using something like `save "2:LR2/BAS"`
    - Power down the machine, remove the SD card, and insert it into your non-vintage machine
    - You can extract files from the image (`hard4-0`) using a utility such as [TRSTools](https://www.trs-80.com/main-emulation-disk-utilities.htm)
- Alternatively, you can make and test changes in the [trs80gp emulator](https://48k.ca/trs80gp.html)
    - *Start it from the command line with these options:* `./trs80gp.exe -m4p -gg -frehd_dir /c/path/to/hard/drive/image/` (using the gitbash shell)
    - Once you are satisfied with your source changes, file saves should be visible in the drive image (make sure the image is writable in the emulator settings). Copy the drive image back to the SD card, insert it back in the TRS-80, and reboot.