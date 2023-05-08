import socket
import traceback
import logging

UDP_IP = "172.100.11.255"
UDP_PORT = 29514

sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
sock.bind((UDP_IP, UDP_PORT))

logging.basicConfig(
    #filename='udawa.log',
    #filemode='a',
    format='%(asctime)s %(levelname)-8s %(message)s',
    level=logging.DEBUG,
    datefmt='%Y-%m-%d %H:%M:%S')

while True:
    try:
        data, addr = sock.recvfrom(1024)
        text = data.decode("utf-8")
        text = text.strip()
        msg = text.split('~');
        if(msg[0] == 'E'):
            logging.error(msg[1] + ": " + msg[2])
        elif(msg[0] == 'W'):
            logging.warning(msg[1] + ": " + msg[2])
        elif(msg[0] == 'I'):
            logging.info(msg[1] + ": " + msg[2])
        elif(msg[0] == 'D' or msg[0] == 'V'):
            logging.debug(msg[1] + ": " + msg[2])
        
    except Exception as e:
        logging.error(traceback.format_exc())
