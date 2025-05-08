import socket
import sys

from interface import HardwareInterface

class ISSILensControl(HardwareInterface):
    """Implements the ISSI lens controller interface"""
    capabilities = {
        "Zoom":["+", "++", "-", "--"],
        "Focus":["+", "++", "-", "--"],
        "Iris":["Open", "Close"]
    }

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.zoom_step_size = 100
        self.focus_step_size = 100
        self.iris_step_size = 100

        self.send("setIrisMode=1")

    def Zoom(self, arg):
        if arg == "-":
            #zoom wide fine
            cmd = "ZoomW=" + str(self.zoom_step_size) + "\n"
        elif arg == "--":
            #zoom wide coarse
            cmd = "ZoomW=" + str(2*self.zoom_step_size) + "\n"
        elif arg == "+":
            #zoom narrow command construction
            cmd = "ZoomN=" + str(self.zoom_step_size) + "\n"
        elif arg == "++":
            #zoom narrow command construction
            cmd = "ZoomN=" + str(2*self.zoom_step_size) + "\n"
        
        self.send(cmd)

    def Focus(self, arg):
        if arg == "-":
            #focus near cmd
            cmd = "FocusN=" + str(self.focus_step_size) + "\n"
        elif arg == "--":
            #focus near cmd
            cmd = "FocusN=" + str(2*self.focus_step_size) + "\n"
        elif arg == "+":
            #focus far cmd
            cmd = "FocusF=" + str(self.focus_step_size) + "\n"
        elif arg == "++":
            #focus far cmd
            cmd = "FocusF=" + str(2*self.focus_step_size) + "\n"

        self.send(cmd)

    def Iris(self, arg):
        if arg == "Open":
            # I must have flipped the wiring
            cmd = "IrisO=" + str(self.iris_step_size) + "\n"
        if arg == "Close":
            # I must have flipped the wiring
            cmd = "IrisC=" + str(self.iris_step_size) + "\n"
        
        self.send(cmd)


if __name__ == "__main__":
    ISSILensControl.command_line()