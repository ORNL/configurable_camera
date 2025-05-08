
from interface import HardwareInterface

class ISSIFocusControl(HardwareInterface):
    """Implements the EF-lens focus control interface"""

    capabilities = {
        "Focus":["+", "++", "-", "--"],
    }

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.step_size = 50
    
    def Focus(self, arg):
        if arg == "+":
            cmd = "moveFocus="+str(self.step_size)+"<CR>\n"
        elif arg == "++":
            cmd = "moveFocus="+str(2*self.step_size)+"<CR>\n"
        elif arg == "-":
            cmd = "moveFocus=-"+str(self.step_size)+"<CR>\n"
        elif arg == "--":
            cmd = "moveFocus=-"+str(2*self.step_size)+"<CR>\n"
        
        self.send(cmd)

if __name__ == "__main__":
    ISSIFocusControl.command_line()
