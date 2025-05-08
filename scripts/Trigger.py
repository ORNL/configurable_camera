import socket
import sys
import platform

# end triggers reset of compressed video, reset triggers start of raw video
# check OS running app, bind correct interface if Windows


class Trigger:

    def __init__(self):
        self.UDP_IP = "127.0.0.1"
        self.UDP_PORT = 2000

        self.os = platform.system()
        self.sock = socket.socket(socket.AF_INET,  # Internet
                                  socket.SOCK_DGRAM,  # UDP
                                  socket.IPPROTO_UDP)
        self.socket_setup()

    def socket_setup(self):
        if self.os == 'Windows':
            address = socket.gethostbyname(socket.gethostname())
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            # TODO: populate IP automatically if Windows
            self.sock.bind((address, 0))

        else:
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            self.sock.bind(('', 0))

    def send(self, message_str):
        message = message_str
        print("Sending %s", message)
        self.sock.sendto(message.encode('ascii'), (self.UDP_IP, self.UDP_PORT))


if __name__ == "__main__":

    trig = Trigger()
    # while True:
    trig.send('test test test')
    string = input("Enter quit code: ")
    trig.send(string)
