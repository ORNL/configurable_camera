# OVERVIEW 

- [Setup](#setup)
	- [Assembling the Camera](#assembling-the-camera)
	- [Field Setup](#field-setup)
- [Operation](#operation)
	- [Running the Camera Software](#running-the-camera-software)
	- [Field Operation](#field-operation)
	- [End of Day Procedures](#end-of-day-procedures)
- [Syncing NTP](#syncing-ntp)
- [NVMe Drives](#nvme-drives)
- [Data Transfers](#data-handling)


This application streams compressed video from Basler Sensors over RTSP, optionally records raw sensor data, and integrates into the existing hardware and software infrastructure of the project.

For more details on _how it works_ instead of _how to use it_, please reference the [App Systems](./App_Systems.md) documentation.

## Setup

### Assembling the camera

Go through the following checklist, some items may already have been completed.

1. Make notes of the following:
    - The lens type ([Lenses](./Hardware.md#lens))
    - The hostname of the jetson
    - Any control hardware
2. Wire control hardware into the enclosure.
3. The jetsons should velcro into the bottom of the enclosure
    - if the control hardware connects to the Jetson's GPIO pins, make sure you've used the correct pins [Diagram for both Orin and AGX Jetsons](https://jetsonhacks.com/nvidia-jetson-agx-orin-gpio-header-pinout/)
4. Connect the hardwired ethernet port on the Jetson to one of the panel mount jacks with a patch cord
    - Label this port on the outside of the box as `NVR`
5. Connect a USB to ethernet adapter to one of the Jetson USB ports
    - Use a USB to dual-ethernet adapter if there is networked control hardware
        - Note: The USB-A port on the Jetson AGXs looks really weird.
6. Connect one of the USB-Ethernet ports to the other panel mount jack with a patch cord
    - Label this port on the outside of the box as `NETWORK`
7. Connect any networked control hardware to the other USB-Ethernet port.
8. Connect a USB-NVMe drive to the jetson
9. Plug in the power jack to the jetson
10. Mount the lens, attach any lens control hardware, remove the lens cap
11. Attach the sensor to the lens and attach heatsinks or fans to the sensor
12. Plug the sensor into one of the USB-C ports on the jetson.
13. Make sure the jetson auto-powers on when you plug in a cable -- there are a couple pins on the bottom that need to be jumpered for this to work. Either look up how to do this or let someone know. In a pinch, swap out the jetson for another one that does power on, or if you absolutely have to, steal a pin jumper from a jetson not in use, and label that jetson as NOT-AUTO-ON with a sticky note or tape or something.
13. Power on the jetson and go through the [installation](../README.md#installation) steps if the software is not already installed
14. If the jetson still has data from previous collections in `/data/rec`, clear it out, but BE CAREFUL.
15. [Format and mount the NVMe drive](#usb-nvme-drive-usage)
    - Note the filepath to the drive -- this is needed for the config file
16. If the jetson has networked control hardware, power it on with a monitor, keyboard, and mouse attached, and manually set the IP address of the ethernet port connected to the control hardware device to an address in the same subnet as the device.
17. Fill out as much of `config.json` as possible with the information available: [Config Documentation](./App_Systems.md#config)

### Field Setup

Info needed:
- Location (station and target)
- TimeMachine ntp time server IP address
- Lens type
- Jetson hostname
- Control hardware info
- Whether the station will record raw footage
- Control Port number (should match HST app)
- Status Broadcast Port number (should match Monitoring App)

Materials needed:
- Rangefinder
- NVR, enclosure, & enclosure internals equipment. 
- Generator
- Long ethernet cables (x2)
    - Check distance to the NVR
- Always good to have smaller patch cables, extra network switch on hand for NVR configuration
- If used with a power box:
    - Power Box
    - Power cable (12V screw-in cable with 2 prongs)
    - Screw-lock 120V extension power cable.
- If used with a standalone power supply:
    - Standalone power supply with long power cord
    - Weatherproof box
- If tripod-mounted:
    - MK2 Tripod (big boi, but not the BMFT)
    - Arca-Swiss Tripod head
    - Sandbag
    - Verify config enclosure has correct mounting plate
- If mast:
    - Big gimbal (should be in mast trailer storage box)

1. Connect the camera port labeled `NVR` to one of the NVR ports
2. Connect the camera port labeled `NETWORK` to a port that can see the network.
3. Connect the configurable power cable to the configurable enclosure. The jetson should power on.
4. Note the following:
    - The name of the target location (e.g. field, mock city, etc.)
    - The position of the camera relative to the subject box or target area.
        - Range (m)
        - Yaw (approximate degrees from North to the target box)
        - Whether the camera is elevated (on a mast or rooftop)
        - Side note: hoping to eventually have GPS units attached to the cameras over USB to automate this. But we'll see.
5. Turn on the NVR. You should be able to ssh into the jetson on the windows command line. `ssh <jetson_hostname>` and type in the password. Then `cd configurable_code`
6. Check that the jetson is synced to the correct time machine ntp time server: run `systemctl status systemd-timesyncd`. There should be a line that says `Status: synchronized to time server <address>`. Verify that this address matches the time machine's IP address. If it doesn't, see [Syncing NTP](#syncing-ntp)
6. Run `./sensor_setup`. This needs to be run each time a new sensor is used with the jetson.
6. Check that the [USB NVMe](#nvme-drive-setup) is correctly mounted, note the filepath
6. Edit the `config.json` file (see [Config Documentation](./App_Systems.md#config) for detailed instructions)
    - Fill out metadata information
    - Edit the Raw Storage path if necessary to match the USB NVMe drive.
    - Make sure that the Control Port and Status Broadcast Port are correct
    - Raw Time Limit should be `600` (seconds)
    - Control IP should be "eth1"
    - RTSP Address should be "eth0"
    - Fill out or copy paste [Hardware Script](./Hardware.md#peripheral-hardware) objects into the list
6. [Run the camera software](#running-the-camera-software) on the jetson.
7. Run the monitoring app on a computer connected to the same network
    - A box should pop up with the info from the camera
8. Access Camera Station on the NVR to set up the connection and start capturing video over RTSP:
    - From the Devices tab, open the Add Device menu.
    - Click the "From Stream URL" button
    - Enter the RTSP Address. You can copy paste this from the output of the program startup (`screen -r camera` to see the startup output), or from the monitoring app. Or just type it in.
    - Set up like any other IP camera
    - Click the Add button in the bottom right of the window
    - Add the camera to whatever Views you want to see, set up a recording schedule, etc.
    - Confirm that the RTSP widget in the monitoring app lights up, showing an active stream.
9. If possible, manually focus the lens on the target.
10. Verify hardware control with the monitoring app.

## Operation

### Running the Camera Software

To start the program:
1. Make sure the camera is properly [assembled](#assembling-the-camera),
[set up](#field-setup), and [configured](./App_Systems.md#config).
2. SSH into the jetson, type `cd configurable_code` to navigate to the software.
3. If it hasn't been done yet for the current sensor, run `./sensor_setup`. This needs to be re-run any time a new sensor is swapped out.
4. Run the startup script: `./run.sh`. You may need to run `chmod +x run.sh` to make it executable.
6. If this doesn't work, make sure the software is [built correctly](../README.md#build) and retry it.
7. You can exit and enter the screen session started by this script with Ctrl-A Ctrl-D and `screen -r camera`.
8. If you're running the monitoring app on another computer on the same network, you should see a status box for this jetson pop up.
9. Make sure the video feed comes up on the NVR if the camera has been set up.

To stop it:
1. SSH into the jetson and run the following:
2. `screen -r`, or `screen -r camera`
3. Ctrl-C
4. `exit`

### Field Operation

These instructions only apply after [Field Setup](#field-setup) has been completed.

1. Run the monitoring app and Camera Station
2. [Start the camera software](#running-the-camera-software)
3. Check that the camera connects to the NVR and shows up on the monitoring app
4. Complete any zoom/focus/position adjustments
5. Set up recordings and views in Camera Station
6. Monitor the cameras
    - RTSP remains active
    - Proctor triggers toggle raw
    - Remaining disk space
7. If disk space is running out:
    1. [Stop the camera software](#running-the-camera-software)
    2. Edit the `config.json` file, set the `"Record Raw"` field to `false`
    3. [Restart the camera software](#running-the-camera-software)

### End of Day Procedures

1. Check remaining disk space for all cameras recording raw
	- Any that have less than a day's worth of storage left (~1TB) should be prioritized for data transfers.
2. [Stop all the cameras](#running-the-camera-software)
3. Unmount USB-NVMe drives for jetsons that need data transferred.
4. Start transfers to consolidated data drives

## Syncing NTP

To see if the jetson is synced to the time server, run:
```
systemctl status systemd-timesyncd
```

If it does not show the time machine IP address, you'll need to edit `/etc/systemd/timesyncd.conf`

Add the Time Machine IP address to the beginning of the `NTP=` space-separated list. Then run

```
sudo systemctl restart systemd-timesyncd
systemctl status systemd-timesyncd
```
To see if your change was successful. 

## NVMe Drives

### NVMe Drive Setup

Installing a USB-NVMe drive:
- Make sure the USB-NVMe drive is assembled according to the instruction booklet.
- The drive should be connected to one of the hardwired USB-C ports on the Jetson.
- Once the drive is plugged in, the LED indicator should light up, and you should be able to see a new ~4T disk called `sda` if you run `lsblk` in a terminal.
- If you do not see it, unplug the drive and rerun `lsblk` to see if the list changes. Then plug it back in. If one of the disks re-appears, use whatever name it has instead.
- Follow the instructions below.

Installing an internal NVMe drive:
- You have to carefully remove the top of the jetson by unscrewing the screws in the feet/standoffs and gently prying off the metal enclosure with the fan in it.
There's a short wire connecting the top and bottom, be careful not to damage it when pulling them apart.
- Plug in and fasten down the springy end of the drive with the small screw.
- Power on the jetson and follow the instructions below.

---

Open a terminal and run `lsblk`.
You should be able to see a 3.7T disk called `sda` or `nvme0n1` in the list of disks and partitions. 

1. Run `sudo fdisk /dev/<disk_name>` to partition the drive.
    - This will start a program in the terminal
    - Type `m` to open the help menu. This will give you a summary of the commands you can use.
    - Type `g` to create a gpd drive map.
    - Type `n` to create a new partition.
        - accept all defaults to use the whole drive's capacity
    - Type `w` to write out the changes to the drive.

2. Rerun `lsblk`. Confirm that a partition called `sda1`, `nvme0n1p1` or something similar now appears under the disk.

3. Run `sudo mkfs -t ext4 /dev/<partition_name>`, being careful to use the partition name (`sda1`, `nvme0n1p1`) to format an ext4 filesystem onto the drive, NOT the disk name (`sda`, `nvme0n1`).

4. If you're using a USB drive, navigate to the `/mnt` directory with `cd /mnt`, if you're using an internal NVMe drive, navigate to the root directory with `cd /`.

5. Make a new directory there called `data` with `sudo mkdir data`

6. Mount the drive partition to the directory.
    - If you're using a USB drive, by running `sudo mount -t ext4 /dev/sda1 ./data`
    - If you're using an internal NVMe, by editing the fstab file: `sudo vim /etc/fstab`
        - Vimstructions(Use emacs/nano if you're a casual):
            - Scroll up or down to the `/dev/root` line with `j` and `k` or the arrow keys
            - `Shift-V` to highlight the whole line
            - `y` to copy what you highlighted
            - `Shift-G` to go to the bottom of the file
            - `p` to paste what you copied
            - `i` to edit the file. Change `/dev/root` in the line you copied to `/dev/nvme0n1p1`. So the full line looks like:

            ``` /dev/nvme0n1p1       /data/                ext4           defaults           0 1 ```
            - `:wq` and hit enter to save and exit, or `:q!` and enter to exit without saving if you messed up.
        - Run `sudo mount /data`

7. Check that it is properly mounted by running `df -h ./data`.

8. Change the permissions with `sudo chown -R <user> ./data`.

9. Create the recording directory: run `mkdir ./data/rec`

10. You'll need to change the filepath of the `"Raw Storage"` directory to `/mnt/data/rec/` or `/data/rec`, depending on where you mounted it, before running the application.

#### USB NVMe Drive Usage
If you want to remove the drive while the Jetson is still powered:
Run `sudo umount /data`. Note the command is `umount`, not `unmount`. Make sure to close any applications using the drive first, including the file browser.
If the Jetson is powered off, you can just remove the drive

### Data Transfers

If swapping USB-NVMe drives is not an option:
[Jetson Data Transfer](https://code.ornl.gov/ghj/jetson_data_transfer)
- We might be able to run this during the day, even while recording.
- The filepath to the data drive on the jetson has to be `/data/rec/`, or the transfer script code needs to be changed for this to work.

We should stagger data transfers -- the jetsons probably have about 3 days worth of storage, we should target transfers for about a third of them each day
