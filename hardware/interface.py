import socket
import traceback
import argparse
import json

class HardwareInterface:
    '''Base class for hardware interface scripts'''

    capabilities = {}

    @classmethod
    def command_line(cls):
        parser = argparse.ArgumentParser(description=cls.__doc__, epilog=json.dumps(cls.capabilities, indent=2))
        parser.add_argument("recv_ip", help="The IP address of the jetson on the network")
        parser.add_argument("recv_port", help="The port at which the jetson should listen for commands for this hardware")
        parser.add_argument("device_ip", help="The IP address of the device (if it has one, otherwise a junk value)")
        parser.add_argument("device_port", help="The port at the device's IP address (if it has one, otherwise a junk value)")
        args = parser.parse_args()

        hardware_interface = cls(**vars(args))
        hardware_interface.start()
        return hardware_interface

    def __init__(self, 
                 recv_ip,
                 recv_port,
                 device_ip,
                 device_port):

        self.UDP_IP = recv_ip
        self.UDP_PORT = int(recv_port)

        self.sockin = socket.socket(socket.AF_INET,
                            socket.SOCK_DGRAM)
        self.sockin.bind((self.UDP_IP, self.UDP_PORT))

        try:
            self.DEVICE_IP = device_ip
            self.DEVICE_PORT = int(device_port)
            self.sockout = socket.socket(socket.AF_INET,
                                    socket.SOCK_DGRAM)
        except:
            self.DEVICE_IP = device_ip
            self.DEVICE_PORT = device_port
            self.sockout = None
        
    def send(self, command):
        if self.sockout is not None:
            command = command.encode()
            self.sockout.sendto(command, (self.DEVICE_IP, self.DEVICE_PORT))
        else:
            print("No device socket.")

    def start(self):
        print(f"Starting {self.__class__} control script.")
        try:
            while True:
                data, addr = self.sockin.recvfrom(1024)
                data = data.decode()
                args = data.strip().split(" ")
                if len(args) == 2:
                    self.execute_command(*args)

        except KeyboardInterrupt:
            self.close()
        except Exception as e:
            print(traceback.format_exc())
            self.close()

    def close(self):
        print(f"Closing {self.__class__} control script.")
        self.sockin.close()
        if self.sockout is not None:
            self.sockout.close()

    def execute_command(self, *args):
        try:
            if args[0] not in self.capabilities:
                print(f"Invalid capability: {args[0]}.")
                return None
            if args[1] not in self.capabilities[args[0]]:
                print(f"Invalid command for capability: {args[0]} {args[1]}.")
                return None

            # get the function defined for the capability and pass the arg
            func = getattr(self, args[0])
            func(args[1])
        except:
            print("Error executing command")
            self.close()


if __name__ == "__main__":
    hi = HardwareInterface.command_line()
