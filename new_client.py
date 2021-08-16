import socket

ip ="10.0.2.2"
port = 7

def isOpen(ip , port):
  s = socket.socket(socket.AF_INET , socket.SOCK_STREAM)
  try:
    s.connect((ip , int(port)))
    s.shutdown(2)
    return True
  except:
    return False


def GetDmaLength(ip, port):

    buffer = 1024
    write = b'2'
    s = socket.socket(socket.AF_INET , socket.SOCK_STREAM)
    s.connect((ip , int(port)))
    s.send(write)
    data = s.recv(buffer)
    data =  int.from_bytes(data,byteorder ='little')
    s.shutdown(2)
    return data

def read(ip , port):
    dma_length = GetDmaLength(ip, port)
    bytes_recv = 0
    write = b"1"
    s = socket.socket(socket.AF_INET , socket.SOCK_STREAM)
    s.connect((ip , int(port)))
    chunks = []
    while bytes_recv < dma_length:
        s.send(write)
        chunk = s.recv(min(dma_length - bytes_recv, 8192))
        chunks.append(chunk)
        bytes_recv = bytes_recv + len(chunk)
    print("You received {} bytes".format(bytes_recv))
    recvdata = b''.join(chunks)
    n = 4
    word_list = [recvdata[i:i + n] for i in range(0, len(recvdata), n)]
    for i in word_list:
        print("Data recived {} in bytes (little endian) or {} in decimal".format(i,int.from_bytes(i,byteorder ='little')))
    return bytes_recv
print(read(ip,port))
