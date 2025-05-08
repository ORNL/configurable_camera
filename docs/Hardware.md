## Sensors

The camera currently works with both mono and color Basler sensors. 
The resolution of all sensors is currently constrained to 1920x1080 pixels, but we will revisit trying to use full sensor resolutions.

The sensor interface is handled by the Gstreamer plugin [gst-plugin-pylon](https://github.com/basler/gst-plugin-pylon/tree/v0.5.1), which allows an attached Basler camera to be used as a Gstreamer source: [`pylonsrc`](other/pylonsrc_examples/pylonsrc.txt). 
We've been using the v0.5.1 tag, because we've had difficulties installing later tags on the jetsons. This needs to be re-explored at some point.

## Lenses

The lens name recorded in the `config.json` file should exactly match one of the values in the `Lens Name` column:

| Lens Name | Lens Model            | Focal Length                |
|-----------|-----------------------------------|-----------------------------|
| Fujinon D60 | Fujinon D60x16.7SR4DE-V23S        | 16.7-1000mm or 33.4-2000mm with 2x extender |
| Kowa  | Kowa LMZ20750AMPDC-XF             | 20-750mm                    |
| Sigma 300 | Sigma 18-300mm f3.5-6.3 DC OS HSM | 18-300mm                    |
| Sigma 600 | Sigma 150-600mm f5-6.3 DG OS HSM  | 150-600mm                   |
| Swarovski 65  | Swarovski 65                      | 750-1800mm                  |
| Swarovski 85  | Swarovski 85                      | 750-1800mm                  |
| Swarovski 95  | Swarovski 95                      | 900-2100mm                  |
| Swarovski 115 | Swarovski 115                     | 900-2100mm                  |
| Tamron 400 | Tamron 18-400mm f/3.5-6.3 Di II VC HLD | 18-400mm               |

## Peripheral Hardware

See the `hardware/` directory for pre-filled JSON hardware script objects.

- ISSI Lens Controller
    - `issi_lens_control.py`
    - ISSI Lens Control Boxes (the blue ones)
    - Used with:
        - Kowa Box Lens
- ISSI Focus Controller
    - `issi_focus_control.py`
    - ISSI Focus Control Box (the black one)
    - Used with:
        - Sigma 600 EF Lens
- GPIO Focus Motor
    - `gpio_focus_motor.py`
    - Custom stepper motor focus ring actuators
    - Uses GPIO Pins 29, 31, 33, 35 for stepper control board IN1-IN4
    - Used with:
        - Sigma 300 
        - Sigma 600
        - Swarovski 65
        - Swarovski 85
        - Swarovski 95
        - Tamron 400
- Fujinon
    - `fujinon.py`
    - Extends the ISSI Lens control code to operate the 2x extender in the Fujinon D60 box lens
    - Uses GPIO pin 29 for the DPDT relay circuit and pin 31 for the repurposed stepper motor driver board.
    - Used with:
        - Fujinon D60

### Potentials
- Exposure Time
    - `exposure_time.py`
    - Adapter for added functionality in `src/udp_server.c` to adjust the Basler sensor exposure time
    - Used with:
        - All basler sensors