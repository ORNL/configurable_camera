
from interface import HardwareInterface

class ISSIFocusMotor(HardwareInterface):
    """Implements the ISSI zoom lens controller motor interface
        repurposed for focus control of the Swarovski 115 lens"""

    capabilities = {
        "Focus":["+", "++", "-", "--"],
    }

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.step_size = 50
    
    def Focus(self, arg):
        # motor is designed for zoom rings, using for focus
        if arg == "+":
            cmd = "moveZoom="+str(self.step_size)+"<CR>\n"
        elif arg == "++":
            cmd = "moveZoom="+str(2*self.step_size)+"<CR>\n"
        elif arg == "-":
            cmd = "moveZoom=-"+str(self.step_size)+"<CR>\n"
        elif arg == "--":
            cmd = "moveZoom=-"+str(2*self.step_size)+"<CR>\n"
        
        self.send(cmd)

if __name__ == "__main__":
    ISSIFocusMotor.command_line()
