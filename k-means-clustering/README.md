# Runbook (onsite)

Runs on the IBM PCjr.

## From power down

- Ensure CF card labeled "PCjr" is in the XT-IDE device (the sidecar on the right side of the machine)
- Power on (rear left)
- Bring up boot device menu when prompted (ESC)
- Select flash drive/XT-IDE device (Sandisk SDCFH-512, option `C`)
- System boots to DOS prompt


### Notes and troubleshooting

- If flash drive is not appearing as a boot option, make sure sidecar is properly seated, the CF card is present and properly seated, and the IDE ribbon cable to the CF adapter is intact and properly seated


## From DOS prompt

- A copy of the demo is on the machine's hard drive. If you need to import fresh code, 
    - Insert USB stick in PCjr Gotek (stick is labeled "PCjr")
    - Twiddle Gotek knob to make sure correct disk image is selected (probably "KMEANS")
    - Copy your fresh file to the hard drive using something like `COPY A:\KMEANS.PAS D:\TP\`
- `cd d:\TP`
- `kmeans5` to run executable
- To edit source,
    - `turbo`
    - Press `F3` to load file. Load `KMEANS5.PAS`. Edit file.
    - Run from IDE (`Ctrl+F9`)


### Notes and troubleshooting

- Screen should fill up with multicolored pixels and crosses
- Demo runs indefinitely


## Making changes

- You can edit code directly on the machine
    - To persist changes off the PCjr, copy the edited source back to `A:\`
    - Extract the saved file from the USB stick on a modern machine using a tool such as [WinImage](https://www.winimage.com/download.htm)
- Alternatively, you can make and test changes in DOSBOX
    - Requires DOS 5.0 and TurboPascal 5.0 installed on a virtual DOS machine
    - Once you are satisfied with your source changes, you can insert the modified source into the floppy image on the USB stick using [WinImage](https://www.winimage.com/download.htm)