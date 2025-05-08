import Jetson.GPIO as GPIO
import time

from issi_lens_control import ISSILensControl

class Fujinon(ISSILensControl):
    """Adapts the ISSILensControl class with additional functionality for the fujinon lens 2x extender"""
    capabilities = {
        "Zoom":["+", "++", "-", "--"],
        "Focus":["+", "++", "-", "--"],
        "Iris":["Open", "Close"],
        "Extender":["1x", "2x"]
    }

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.DPDT_RELAY = 29
        self.MOTOR_DRIVER = 31
        GPIO.setmode(GPIO.BOARD)
        GPIO.setup(self.DPDT_RELAY, GPIO.OUT)
        GPIO.setup(self.MOTOR_DRIVER, GPIO.OUT)
        time.sleep(1)

    def close(self):
        GPIO.cleanup()
        super().close()

    def motor_on(self):
        GPIO.output(self.MOTOR_DRIVER, GPIO.HIGH)

    def motor_off(self):
        GPIO.output(self.MOTOR_DRIVER, GPIO.LOW)

    def config1x(self):
        GPIO.output(self.DPDT_RELAY, GPIO.LOW)

    def config2x(self):
        GPIO.output(self.DPDT_RELAY, GPIO.HIGH)
    
    def all_off(self):
        GPIO.output(self.MOTOR_DRIVER, GPIO.LOW)
        GPIO.output(self.DPDT_RELAY, GPIO.LOW)

    def Extender(self, arg):
        if arg == "1x":
            self.config1x()
            self.motor_on()
            time.sleep(1)
            self.all_off()
        elif arg == "2x":
            self.config2x()
            self.motor_on()
            time.sleep(1)
            self.all_off()


if __name__ == "__main__":
    Fujinon.command_line()