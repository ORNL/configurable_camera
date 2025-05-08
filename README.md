# configurable_camera

Written by Gavin Jager and Greyson Jones, with help from interns Shane Menzigian, Laura Smith, and Christian Tongate.

Acknowledgement and thanks to Christi Johnson and David Cornett.
Based on a previous Python-based implementation by David Bolme.

## Overview

### Helpful Links
- [Installation](#installation)
- [User Manual](docs/Manual.md)

### Repository Structure
| File/Directory | Description |
| - | - |
| `docs/`       | documentation on app systems and hardware          |
| `hardware/`   | python scripts for peripheral hardware control     |
| `include/`    | location for all header (.h) files                 |
| `logs/`       | logs created by the startup script go here.        |
| `scripts/`    | some installation and testing scripts              |
| `src/`        | location for all source code (.c) files            |
| `tests/`		| tests (probably out-of-date)                       |
| `config.json`	| configuration file for system settings             |
| `Makefile`	| main and test compilation recipes                  |
| `pipeline.txt`| the gstreamer pipeline used by the RTSP server     |
| `README.md`	| you are here                                       |
| `run.sh`      | startup script                                     |

### Compiled Executables
```
sensor_setup    -- run whenever a new sensor is used
main            -- called by run.sh
```

### General Description

This code was written for a custom camera platform with interchangeable lenses and sensors developed for the IARPA-BRIAR project in ORNL's role as T&E partner. This software provides a consistent interface with multiple sensor formats and lens control schemes. It also integrates with the other commercial and custom systems used on the project and records specialized data and metadata. The software runs on Jetson AGX platforms using Basler 2D sensors connected over USB.

The code uses Gstreamer and Glib libraries to create and manage an RTSP server used to stream video data. It uses a Gstreamer plugin developed by Basler to pull frames from a sensor into a media pipeline, which is configured to handle the specific sensor format. The media pipeline compresses the video to h264, which is then streamed using over RTSP once a client connects to the camera. Status information from the camera is continuously broadcast over UDP on a separate network. Additionally, UDP commands can be sent over the network to the camera to control sensor and lens settings as well as toggle short recordings of the uncompressed frames from the sensor. 

This is a pretty comprehensive overhaul of the old python code, but still retains most of the base functionality and the ability to interface with the [HST app](#related-software). Here are the main differences:

- The configurable cameras can now stream directly to a network video recorder or other RTSP client
- All control and metadata details needed for camera operation are configured in `config.json` (see [App Systems - Config](docs/App_Systems.md#config))
- Raw recording is optional
- An additional [hardware control interface](docs/App_Systems.md#peripheral-hardware-control) is used to adapt peripheral hardware (focus motors, lens controllers, gimbals, etc.) to a consistent UDP interface defined in [config.json](docs/App_Systems.md#config). 
- Updated compatibility with the [HST App](#related-software) to selectively trigger raw recording based on location/target.
- The [Monitoring Application](#related-software) was rewritten in LabView:
    - Got rid of the viewer (but might add it back in...).
    - Implements a user interface for hardware (focus motor, etc.) control.
    - Added some additional status checks and displays
- Some of the metadata formatting has changed to simplify curation

## Installation

This software is meant for installation on NVIDIA Jetson AGX Xaviers and Orins, using Basler acA sensors.

The Jetsons have Gstreamer installed by default.

### Dependencies

```
sudo apt update
sudo apt upgrade
sudo apt install libgstrtspserver-1.0-0 libgstrtspserver-1.0-dev screen
```

#### Install/Upgrade Baser Pylon SDK:

1. Check the current installation (if there is one)
    - `pylon --version` or dig around in `/opt/pylon`
    - If the version is ~7.4 we're good, proceed to [Install Gstreamer Pylon Source](#install-gstreamer-pylon-source).
2. Get the newest version off the software drive (ask somebody), or download it if the drive is not available:
- [Basler Downloads](www2.baslerweb.com/en/downloads/software-downloads/)
- Download the newest Pylon Camera Software Suite Linux x86 (64 Bit) - Debian Installer Package

In the terminal, navigate to the ~/Downloads directory and run the follwing to extract the .deb installer files into a new directory

```
mkdir pylon_setup
tar -C pylon_setup -xvzf pylon_*.tar.gz
cd pylon_setup
sudo apt-get install ./pylon_*.deb ./codemeter*.deb
```

This will install the pylon SDK.

Run `echo $PYLON_ROOT` and verify that /opt/pylon is returned

#### Install Gstreamer Pylon Source

If there's a copy of the repo on the software drive, copy it to the home directory, otherwise go to the [gst-plugin-pylon](github.com/basler/gst-plugin-pylon) repo, and copy the https address to clone it. Open a terminal and clone the repo into the home directory.

```
cd ~
git clone https://github.com/basler/gst-plugin-pylon.git
```

Install the plugin:
```
cd gst-plugin-pylon
sudo apt remove meson ninja-build
sudo apt install ninja-build
sudo apt install python3-pip
sudo -H python3 -m pip install meson --upgrade
meson setup builddir --prefix /usr/
ninja -C builddir
ninja -C builddir test
sudo ninja -C builddir install
```

If everything worked, you should be able to run:

```
gst-inspect-1.0 pylonsrc
```

It will print the specs for the gstreamer element.

### Build

```
cd configurable_code
make all
```

This will compile two executables called `main` and `sensor_setup` and put them in the `configurable_code` directory.

Builds are managed with a [Makefile](Makefile), which defines build/compilation recipes for development, deployment, and testing.

During development use `make dev_all` -- compiles with added warnings and things, but not optimized.

### Other recommended software

- VS Code

## Roadmap For Improvements
1. Add a remote shutdown function to the UDP server
2. Implement a web interface for configuration and startup
    - JSON HTML editor for config.json, Python simplehttpserver for a quick and dirty solution
    - Heimdall for easy webpage access.
3. Constrain lens names using a master list
4. See if there's a workaround to get acaA2040-90 temperature information.
4. Generate Hardware Script JSON from hardware scripts themselves.
    - autofill script path, device info, etc, based on name.
5. Pass sensor metadata through the pipeline using Chunk data and the pylonmetadata.h stuff, get frame-aligned metadata.
    - Need to square this with bayer2rgb, extra data in buffers throws this off.
7. See if there's a better way to deal with bayer stuff.
8. Add support for non-Basler sensors (general sensor interface)

## Related Software

This software was designed for compatibility with two other systems written and used on the BRIAR project. At the time of this commit, these apps have not been made publicly available, but their function as it relates to the configurable_camera code should be easy to replicate.

- _BRIAR HST App_: Will update with public facing repo once open-sourced. Used by proctors when an activity in view of the camera is started or stopped; sends raw recording trigger messages to the [UDP interface](./docs/App_Systems.md#udp-command-and-raw-recording-toggle-receiver).
- _Camera Monitoring App_: Lab-view implementation of a monitoring app. Will update with public facing repo or include a version of this software in this repo. Receives [broadcast status messages](./docs/App_Systems.md#udp-status-broadcast) and implements user interfaces for hardware control.


## Notes

autovideosink doesn't work on the jetsons. Use xvimagesink.

Example pipeline to play a raw bayer video:

`gst-launch-1.0 filesrc location=./1695670982.raw blocksize=12288000 ! video/x-bayer,format=rggb,width=4096,height=3000,framerate=30/1 ! bayer2rgb ! videoscale ! video/x-raw,height=1920,width=1080 ! videoconvert ! xvimagesink`

the filesrc blocksize argument should be the number of bytes read into a gstreamer buffer, this should be the number of pixels per frame (width * height)
