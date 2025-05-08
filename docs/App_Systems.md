# APP SYSTEMS OVERVIEW

# Configuration

There are three important configuration files used by the program:
 - `config.json`:
    - physical setup details like range, target location, lens etc.
    - network interfaces for rtsp and control
    - control of raw recording
    - peripheral hardware (focus motors, box lenses)
 - `sensor.json`:
    - sensor properties: resolution, initial values for exposure time and gain
 - `pipeline.txt`:
    - gstreamer pipeline description, element settings


## Config JSON
The [`config.json`](../config.json) file in the top level of the `configurable_code` should be edited for each camera system with details like the range, lens, and associated collection location. The config.json file also contains details of the peripheral hardware that will be controlled by the jetson (see [Peripheral Hardware Control](#peripheral-hardware-control)).

We're working on a more convenient, value-constrained way to set up the `config.json` file in the future.

This file is read when the program is run, and controls/affects several parts of the program, so please take care to ensure it is written correctly.

### Config JSON Template
```
{
    "Metadata":
    {
        "Name":             str,
        "Location":         str,
        "Range":            int,
        "Yaw":              int,
        "Elevated":         bool,
        "Lens Name":        str,
        "Focal Length":     int,
        "Collection ID":   	str  (e.g. "BGC5")
    }
    "Control Data":
    {
        "Record Raw":               bool,
        "Raw Storage":              str, (probably "/data/rec")
        "Control IP":               str, (either an IP address or a network interface name, e.g. "eth0")
        "Control Port":             int,
        "Status Broadcast Port":    int,
        "Raw Time Limit":           int, 
        "RTSP Address":             str, (either an IP address or a network interface name, e.g. "eth1")
        "Hardware Scripts":[
            {
                "Name":             str,
                "Filepath":         str,
                "Listener Port":    str,
                "Device IP":        str,
                "Device Port":      str,
                "Capabilities": [
                    {
                        "Name":str,
                        "Commands":[str]
                    },
                ]
            }
        ]
    }
}
```

### Metadata config

This data will make up the bulk of the `Metadata` struct, be recorded by the program on startup, and be included in the UDP sent in status broadcasts.
1. `"Name"`: This is the config number of the enclosure, e.g. `"config1"`
1. `"Location"`: This defines which area the camera will be focused on, and is used to selectively trigger raw recording for those cameras from the corresponding proctor laptop. The location should be all lowercase with no spaces, and if associated with a particular zone at a location, the zone number should be appended to the location without spaces (e.g. `"field"`, `"mockcity1"`, `"mockcity2"`). Make sure the location names are consistent with the names in the proctor app trigger messages.
2. `"Range"`: approximately how far away the camera is from where it's focused in meters. Be sure to use the rangefinder to determine this value during setup.
3. `"Yaw"`: From the camera's perspective, the compass degrees from north to the target it's focused on. Use your phone's compass app to determine this value during setup.
4. `"Elevated"`: true if the camera is on a mast or rooftop, false if it is on the ground.
5. `"Lens Name"`: the name of the lens (see [Lenses](Hardware.md#lenses))
6. `"Focal Length"`: The focal length in milimeters of the lens being used. Use the maximum focal length listed in [Lenses](Hardware.md#lenses) for now.
7. `"Collection ID"`: The name of the collection (e.g. "BGC5")

### Control Data config

This data will make up the bulk of the program control flow settings, most of which will end up in a struct called `ControlData` (see `include/config.h`).
1. `"Record Raw"`: set this to `true` if the camera should record raw when it receives trigger messages from the proctor app.
2. `"Raw Storage"`: set this to be the base directory for all raw video and metadata recording. This should ideally be `"/data/rec"`
3. `"Control IP"`: This is the identity of the jetson on the network. For ease of use, the value here can either be the actual IP address of the jetson, or, more simply, the name of the network interface of the physical ethernet port that is connected to the network (e.g "eth0", "eth1", "eth2").
4. `"Control Port"`: This is the port that receives trigger messages from the proctor laptops. Make sure this matches the port in the [HST App Raw Trigger]() -- Will update later with valid link.
5. `"Status Broadcast Port"`: This is the port that broadcasts status messages. Make sure this matches the port used by the Monitoring App
6. `"Raw Time Limit"`: the maximum length of a raw recording in seconds. Default: 600
7. `"RTSP Address"`: This is the identity of the ethernet connection to the NVR. The value can either be the actual IP address used by this connection, or the name of the network interface of the physical ethernet port that is connected to the NVR (e.g. "eth0", "eth1", "eth2").
8. `"Hardware Scripts"`: see [Peripheral Hardware Control](#peripheral-hardware-control)

## Sensor JSON

The `sensor_setup` executable must be run every time a new sensor is connected to the jetson. `sensor_setup` connects to the basler sensor and reads information necessary for setup, then writes it to a file: `configurable_code/sensor.json`. This process would normally be done at the beginning of the main program, but it's a quirk of the `pylonsrc` gstreamer element that it seemingly can only be created once in a program. 

The `sensor.json` values are parsed into the pipeline description at the beginning of the main program.

The values for exposure time and gain are also edited by the program when those values are set on the sensor. This was done so that the most recent settings would persist when the program needs to be restarted.

TODO: include sensor.json template. Make the file a permanent part of the repo.

## Pipeline Description

This program uses the Gstreamer framework to create and configure media pipelines from the sensor to serve RTSP and record raw video locally. The RTSP stream is handled with the GstRTSPServer framework, which defines media factories (pipelines) which are created when an RTSP client (ideally, the NVR) requests a stream.

The pipeline is defined in `pipeline.txt`, allowing for easy editing without having to recompile.

## Metadata

This program will record three different types of metadata:
1. Situational metadata
	- a copy of `config.json` renamed to `<timestamp>_startup.json`
	- this is recorded on startup of the program
2. Sensor metadata
	- A JSON file named `<timestamp>_sensor.json` containing:
		- Model
		- Serial Number
		- Color/Mono
		- Pixel Color Filter
		- Pixel Bit Depth
		- Gamma
		- Width
    - Height
		- Horizontal/Vertical Binning
		- Horizontal/Vertical Binning Mode (avg/sum)
		- Shutter Mode
	- this is recorded every time the pipeline starts
3. Frame-level metadata
	- List:
		- Timestamp
		- Gain
		- Auto Gain
		- Exposure
		- Auto Exposure
		- Black Level
		- Auto Target Brightness
		- Frame Rate
		- Sensor Temperature
	- Frame-level metadata is recorded continuously in csvs
	- a new csv is started each time a start or stop trigger is received or the maximum raw recording time limit is reached


## UDP Status Broadcast
JSON Template:
```
{
  "Name":str,
  "Hostname":str,
  "IP Address":str,
  "Control Port":int,
  "Location":str,
  "Range (m)":int,
  "Sensor": str,
  "Sensor Temperature":int,
  "Sensor Temperature State":str,
  "Exposure Time":int,
  "Gain":int
  "Lens":str,
  "RTSP Address":str,
  "RTSP":boolean,
  "Raw":bool,
  "Recording Directory":str,
  "Memory Remaining (GB)":int,
  "CPU Temperature":int,
  "Timestamp":int,
  "Hardware":[
    {
      "Name":str,
      "Capabilities":[
        {
          "Name":str,
          "Commands":[str,]
        }
      ]
    }
  ]
}
```

## UDP Command and Raw Recording Toggle Receiver

Broadcast UDP messages that will be used to control the jetsons (specifically start/stop raw recording) should be formatted as:

`<label> <command> <timestamp> <addl. data>`

It should be a space delimited ascii string. 

The `<label>` field can be used to constrain the effects of broadcast UDP messages to specific jetsons without using their IP addresses/subnets. If `<label>` matches the hostname of the jetson, the `"Location"` field specified in their config file, or `all`, the command specified by `<command>` will be executed.

The `<command>` field can be one of: `start`, `stop`

- `start`: this will begin a new raw video recording and create a file in the recording directory called `<timestamp>_start.txt` that contains the full string received, including the label and additional data fields.

- `stop`: this will end raw recording if one is in progress, and will create a file in the recording directory called `<timestamp>_stop.txt` that contains the full string received, including the label and additional data fields.

## Peripheral Hardware Control

Peripheral hardware control is managed by separate scripts written in Python. These scripts are run by the main application at startup.
These scripts are located in the `hardware/` directory in this repository.

See [Hardware](Hardware.md#peripheral-hardware) for completed & planned interfaces

The UDP interface that the scripts define for the hardware should be documented by the script writer. A hardware script object interface definition should have the following format:

```
{
	"Name":str,
	"Filepath":str, 
	"Listener Port":str,
	"Device IP":str,
	"Device Port":str,
	"Capabilities": [
		{
			"Name":str,
			"Commands":[str]
		}
	]
}
```

A JSON object using this template must be added to the `"Hardware Scripts"` list in the `config.json` file for every piece of hardware controlled by a particular jetson. 

This JSON will be used by the main application to run the individual hardware scripts as separate processes, and it is also broadcast over UDP by each jetson to define control interfaces in the monitoring app.

`"Name"` should be a description of the hardware (e.g. Custom Focus Motor, or ISSI Lens Controller, etc.)

`"Filepath"` should be the path to the corresponding script used to adapt the hardware interface relative to the `configurable_code` directory (e.g. `hardware/issi_focus_control.py`).

`"Listener Port"` defines the port number where the UDP socket will receive control messages.

If the device does not have an IP address (for example, the custom focus motors are controlled with the GPIO pins)
the `"Device IP"` and `"Device Port"` fields should be set to `"None"` or some other junk value,
or they can be reused for other purposes if the hardware requires some other configuration.

`"Device IP"` is the IP address of the hardware that will be controlled. 

`"Device Port"` is the port at which a networked hardware device receives UDP messages.

### Writing Hardware Control Scripts

Scripts are implemented in Python 3 and should use the `HardwareInterface` class defined in `interface.py` as a base class. There are several examples in the `hardware/` directory currently.

To create a new hardware interface, subclass `HardwareInterface` in a new file. Write a docstring for the class. Create a class attribute called `capabilities` and write a dictionary of lists of Commands keyed by capability Names. E.g. if you were implementing a focus motor:

```
class FocusMotor(HardwareInterface):
  capabilities = {
    "Focus":["+", "++", "-", "--"]
  }
  def __init__(self, *args, **kwargs):
    super().__init__(*args, **kwargs)
    ...
```

The `__init__` function should accept *args and **kwargs, and call `super().__init__(*args, **kwargs)`. Define any relevant attributes -- step sizes, configurations, etc. in `__init__`.

To implement a capability, write a class method named called the capability name. E.g. if you're implementing "Focus", write a method:
```
def Focus(self, arg):
  ...
```

This method should handle all commands listed in the `capabilities` under that Name as string arguments.

Be sure to call the `.command_line()` method of the class if the file is run as main. E.g:

```
if __name__ == "__main__":
  FocusMotor.command_line()
```

Finally, add a json file with the same name as your script to the `hardware/` directory with the default details that should be used in the master `config.json` file. This is so that JSON can be copy pasted in without having to manually enter values. Improvements to this system are on the application's roadmap, I know it's bad, sorry.

By default, HardwareInterface subclasses accept the following command line arguments:
- `[Control IP]` -- The main program will pass in the IP address of the jetson
- `[Listener Port]` -- This is the `"Listener Port"` field in the hardware script JSON.
- `[Device IP]` -- This is the `"Device IP"` field in the hardware script JSON.
- `[Device Port]`  -- This is the `"Device Port"` field in the hardware script JSON. 

This will create a UDP listener socket at the IP address and port, which accepts string messages.
If the hardware is controlled over ethernet, it will create a UDP socket from the `[Device IP]` and `[Device Port]` fields if possible.

The UDP listener should accept a space delimited string of two values, and the possibilities for these values should be defined in the `"Capabilities"` field of the hardware script JSON.
The string format is:

```
<name> <command>
```

Where `<name>` matches one of the capabilities' `"Name"` fields, and `<command>` matches one of the corresponding `"Commands"`.

For example, a hardware script with the following `config.json` definition:

```
{
	"Name":			"ISSI Focus Motor",
	"Filepath":		"issi_focus.py", 
	"Listener Port":"3456",
	"Device IP":	"192.168.1.192",
	"Device Port":	"4893",
	"Capabilities": [
		{
			"Name":"Focus",
			"Commands":["+", "-"]
		}
	]
}
```
should start a udp listener on port 3456, and implement the ISSI focus control interface for the device at 192.168.1.192:4893 so that when it receives `Focus +` or `Focus -` over UDP, it will adjust the focus by a suitably small increment.
