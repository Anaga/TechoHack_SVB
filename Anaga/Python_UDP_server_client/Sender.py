import time
import logging

from socket import *
from struct import *
UDP_PORT = 1555
#
CMDS= dict(
    CMD_READ_SENSORS    = 1,
    CMD_WRITE_SETPOINT  = 2,
    CMD_READ_SETPOINT   = 3,
    CMD_READ_POSITION   = 4,
)
class PDU:
    transaction_id = 0
    group_id = 0
    source = 0
    dest = 0
    command = 0
    data_len = 0
    data = 0
    
cs = socket(AF_INET, SOCK_DGRAM)
cs.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
cs.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
cs.sendto('Tissfdghfgjhvsfgt', ('255.255.255.255', UDP_PORT))
    
transactionID = 0;

class ESP_Sender():
   def __init__(self, logger):
     self.__logger   = logger
      
   def reqestLight(self, incoming_list):
     print "request light"
     self.__logger.info("reqestLight: ")
     cs.sendto('ligh req', ('255.255.255.255', UDP_PORT))
     
   def setPoint(self, incoming_list):
     mess = ''
     if len (incoming_list) < 2:
         mess = "setPoint shall have a value"
         self.__logger.info(mess)
         print mess
         return
         
     val = incoming_list[1]
     mess = "Set point to %s"%val
     self.__logger.info(mess)
     print mess
     cs.sendto('set Point', ('255.255.255.255', UDP_PORT))
          

###############################################################################################
# Code to run ESP environment
# Sender script for SVB
###############################################################################################
if __name__ == "__main__":
     #scriptutil.configLoggingHandlers("./Sender.log", False)
     logger = logging.getLogger("Sender")   
     esp_send = ESP_Sender(logger)   
     menu = "Sender menu: Enter choise and then CR.\n" + \
          "rl   - Reqest Ligth \n" + \
          "sp X - SetPoint to X \n" + \
          "m    - Show this menu again\n" + \
          "q    - quit\n"

     print menu
   
     key = raw_input()
     #wait until user wants to quit
     while key != "q":
         menu_choice_list = key.split()
         menu_comand = menu_choice_list[0]
          
         if (menu_comand == "rl"):
             esp_send.reqestLight(menu_choice_list)   
            
         if (menu_comand == "sp"):
             esp_send.setPoint(menu_choice_list)   

         if (menu_comand == "m"):
             print menu
         
         time.sleep(0.25)
         newKey = raw_input()
         if (len(newKey) > 0):
            key = newKey
         