import Jetson.GPIO as GPIO
from time import sleep

from interface import HardwareInterface

class GPIOFocusMotor(HardwareInterface):
    "Custom focus GPIO stepper motor implementation"

    capabilities = {
        "Focus":["+", "++", "-", "--"]
    }

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        # GPIO setup
        self.IN1 = 29
        self.IN2 = 31
        self.IN3 = 33
        self.IN4 = 35
        GPIO.setmode(GPIO.BOARD)
        GPIO.setup(self.IN1, GPIO.OUT)
        GPIO.setup(self.IN2, GPIO.OUT)
        GPIO.setup(self.IN3, GPIO.OUT)
        GPIO.setup(self.IN4, GPIO.OUT)
        sleep(1)

        self.STEP_SIZE = 50
        self.state = 0

    def close(self):
        try:
            GPIO.cleanup()
            super().close()
        except KeyboardInterrupt:
            GPIO.cleanup()
            super().close()

    def Focus(self, arg):
        if arg in ["+", "+\n"]:
            self.spin(self.STEP_SIZE, 1)
        elif arg in ["++", "++\n"]:
            self.spin(2*self.STEP_SIZE, 1)
        elif arg in ["-", "-\n"]:
            self.spin(self.STEP_SIZE, -1)
        elif arg in ["--", "--\n"]:
            self.spin(2*self.STEP_SIZE, -1)

    def spin(self, steps, direction):

        for step in range(steps):
            self.switch(self.state)
            self.state = (self.state + direction) % 8
            sleep(0.0008)

        self.switch(None)

    def switch(self, phase):
        #these are the configurations for each "step" in a single rotation

        if phase == 0:
            GPIO.output(self.IN1, GPIO.LOW)
            GPIO.output(self.IN2, GPIO.LOW)
            GPIO.output(self.IN3, GPIO.LOW)
            GPIO.output(self.IN4, GPIO.HIGH)

        elif phase == 1:
            GPIO.output(self.IN1, GPIO.LOW)
            GPIO.output(self.IN2, GPIO.LOW)
            GPIO.output(self.IN3, GPIO.HIGH)
            GPIO.output(self.IN4, GPIO.HIGH)

        elif phase == 2:
            GPIO.output(self.IN1, GPIO.LOW)
            GPIO.output(self.IN2, GPIO.LOW)
            GPIO.output(self.IN3, GPIO.HIGH)
            GPIO.output(self.IN4, GPIO.LOW)

        elif phase == 3:
            GPIO.output(self.IN1, GPIO.LOW)
            GPIO.output(self.IN2, GPIO.HIGH)
            GPIO.output(self.IN3, GPIO.HIGH)
            GPIO.output(self.IN4, GPIO.LOW)

        elif phase == 4:
            GPIO.output(self.IN1, GPIO.LOW)
            GPIO.output(self.IN2, GPIO.HIGH)
            GPIO.output(self.IN3, GPIO.LOW)
            GPIO.output(self.IN4, GPIO.LOW)

        elif phase == 5:
            GPIO.output(self.IN1, GPIO.HIGH)
            GPIO.output(self.IN2, GPIO.HIGH)
            GPIO.output(self.IN3, GPIO.LOW)
            GPIO.output(self.IN4, GPIO.LOW)

        elif phase == 6:
            GPIO.output(self.IN1, GPIO.HIGH)
            GPIO.output(self.IN2, GPIO.LOW)
            GPIO.output(self.IN3, GPIO.LOW)
            GPIO.output(self.IN4, GPIO.LOW)

        elif phase == 7:
            GPIO.output(self.IN1, GPIO.HIGH)
            GPIO.output(self.IN2, GPIO.LOW)
            GPIO.output(self.IN3, GPIO.LOW)
            GPIO.output(self.IN4, GPIO.HIGH)

        else:
            GPIO.output(self.IN1, GPIO.LOW)
            GPIO.output(self.IN2, GPIO.LOW)
            GPIO.output(self.IN3, GPIO.LOW)
            GPIO.output(self.IN4, GPIO.LOW)


if __name__ == "__main__":
    GPIOFocusMotor.command_line()