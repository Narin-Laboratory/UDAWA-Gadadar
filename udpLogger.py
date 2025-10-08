import socket
import sys
import datetime

if len(sys.argv) < 3:
    #print("Usage: python log_viewer.py <UDP_IP> <UDP_PORT>")
    #sys.exit(1)
    UDP_IP = "0.0.0.0"
    UDP_PORT = 29514
else:
    UDP_IP = sys.argv[1]
    UDP_PORT = int(sys.argv[2])

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Log viewer listening on {UDP_IP}:{UDP_PORT}")

while True:
    data, addr = sock.recvfrom(1024) 
    log_message = data.decode('utf-8')
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    # Get IP from addr and format output
    sender_ip = addr[0]
    print(f"{timestamp} - {sender_ip} - {log_message}", end="") 
