import socket
import struct 

class RapidClient(object):

  def connect(self, host, port):
    self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
    self.sock.connect((host, port))

  def send_recv(self, data):
    magic = socket.htons(11111)
    version = 0
    cmd = 0
    sqid_high = 0
    sqid_low = 0
    size = socket.htonl(16 + len(data))

    msg = struct.pack("HBBIII", magic, version, cmd,\
      sqid_high, sqid_low, size)

    msg += bytes(data,encoding='utf-8')

    self.sock.send(msg);

    res = self.sock.recv(16);
    header = struct.unpack("HBBIII", res)

    if magic != header[0] :
      raise ValueError("package header magic error:%d" % header[0])
    if 0 != header[1] :
      raise ValueError("package version not match:%d" % header[1])
    if 0 != header[2] :
      raise ValueError("package cmd not match:%d" % header[3])
    if 0 != header[3] :
      raise ValueError("package sqid_high not match:%d" % header[3])
    if 0 != header[4] :
      raise ValueError("package sqid_low not match:%d" % header[3])
    if 0 == (header[5] - 16) :
      return ""
    res = self.sock.recv(header[0] - 16);
    return res

if __name__ == '__main__':
  client = RapidClient();
  client.connect("localhost", 9091)

  testdata="method=ping"

  print (client.send_recv(testdata));