#!usr/bin/env python
# -*- coding: utf-8 -*-

import socket
import json
import binascii
import sys
import select
import threading
import time

DataEsp1 = { 0 }

class DataEsp:
    class Card:
        def __init__(self, idCard, flagAccess):
            self.idCard = idCard
            self.flagAccess = flagAccess
    class Esp:
        def __init__(self, idEsp, flagAlert, flagAlertSignaling):
            self.TimerAlert = threading.Timer(20.0, startAlert, [idEsp])
            self.idEsp = idEsp
            self.flagAlert = flagAlert
            self.flagAlertSignaling = flagAlertSignaling
            self.cards = [DataEsp.Card(3334111444, 0), DataEsp.Card(3414231324, 1), DataEsp.Card(3411111124, 0)]
    def __init__(self):
        self.esps = []
    def registrationId(self, idEsp):
        for x in self.esps:
            if idEsp == x.idEsp:
                x.flagAlertSignaling = 1
                return 1
        try:
            self.esps.append(DataEsp.Esp(idEsp,0,1))
            return 1
        except:
            return 0
    def checkIdCard(self, idEsp, idCard):
        for x in self.esps:
            if idEsp == x.idEsp:
                for y in x.cards:
                    if idCard == y.idCard:
                        if y.flagAccess == 1:
                            self.abortTimerAlert(idEsp)
                            return True
        return False
    def checkAlert(self, idEsp):
        for x in self.esps:
            if idEsp == x.idEsp:
                if x.flagAlert == 1:
                    return True
        return False
    def checkAlertSignaling(self, idEsp):
        for x in self.esps:
            if idEsp == x.idEsp:
                if x.flagAlertSignaling == 1:
                    return True
        return False
    def turnAlert(self, idEsp, flagAlert):
        for x in self.esps:
            if idEsp == x.idEsp:
                x.flagAlert = flagAlert
    def turnAlertSignaling(self, idEsp, flagAlertSignaling):
        for x in self.esps:
            if idEsp == x.idEsp:
                x.flagAlertSignaling = flagAlertSignaling
    def startTimerAlert(self, idEsp):
        for x in self.esps:
            if idEsp == x.idEsp:
                x.TimerAlert.start()
    def abortTimerAlert(self, idEsp):
        for x in self.esps:
            if idEsp == x.idEsp:
                x.TimerAlert.cancel()
                x.TimerAlert = threading.Timer(20.0, startAlert, [idEsp])

#TODO /\ exceptions
def startAlert(idEsp):
    globalDB.turnAlert(idEsp, 1)
    globalDB.abortTimerAlert(idEsp)
    print('IdEsp[',idEsp,'] ALERT ALERT ALERT CALL THE POLICE')

def regEsp(sock, recvJson):
    #tempJson = json.loads({"sign_esp":len(idsEsp)})
    sock.send('{"sign_esp":'.encode() + str(globalDB.registrationId(recvJson["reg_esp"])).encode() + '}'.encode() )
    
def checkId(sock, recvJson):
    if globalDB.checkAlertSignaling(recvJson["id_esp"]):
        if globalDB.checkIdCard(recvJson["id_esp"], recvJson["check_id_card"]):
            globalDB.turnAlertSignaling(recvJson["id_esp"], 0)
            sock.send(b'{"check_result":1}')
        else:
            globalDB.turnAlert(recvJson["id_esp"], 1)
            sock.send(b'{"check_result":0}')
            globalDB.turnAlertSignaling(recvJson["id_esp"], 0)
    else:
        if globalDB.checkIdCard(recvJson["id_esp"], recvJson["check_id_card"]):
            globalDB.turnAlertSignaling(recvJson["id_esp"], 1)
            sock.send(b'{"check_result":1}')
            if globalDB.checkAlert(recvJson["id_esp"]):
                globalDB.turnAlert(recvJson["id_esp"], 0)
        else:
            sock.send(b'{"check_result":0}')
                
    
def packageService(sock, data):
    if not data:
        return 0
    try:
        recvJson = json.loads(data.decode("utf-8"))
    except ValueError:
        #print('Incorrect json format(packageSevice)')
        raise Exception('Incorrect json format(packageSevice)')
    if "check_id_card" in recvJson:
        checkId(sock, recvJson)
    elif "reg_esp" in recvJson:
        regEsp(sock, recvJson)
    elif "is_alert" in recvJson:
        if recvJson["is_alert"] == 1:
            globalDB.turnAlert(recvJson["id_esp"], 1)
            
            startAlert(recvJson["id_esp"])
        else:
            globalDB.startTimerAlert(recvJson["id_esp"])


def recvAll(sock, n):
    data = bytearray()
    while len(data) < n:
        packet = sock.recv(n - len(data))
        if not packet:
            return None
        data.extend(packet)
    return data

def globalRecv(sock):
    raw_msglen = recvAll(sock, 2)
    if not raw_msglen:
        return None
    #print('hex', binascii.hexlify(raw_msglen))\
    try:
        msglen = int(raw_msglen.decode("utf-8"))
    except ValueError:
        raise Exception('Incorrect int format(globalRecv)')
    print('length', msglen)
    return recvAll(sock, msglen)

globalDB = DataEsp()
sock = socket.socket()
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(('', 9090))
print('Socket server create')
while True:
    sock.listen(1)
    print('waiting client...')
    conn, addr = sock.accept()
    print('client connected:', addr)
    data = None
    try:
        data = globalRecv(conn)
    except:
        print('[Exception]', sys.exc_info())
    if data:
        try:
            packageService(conn, data)
        except:
            print('[Exception]', sys.exc_info())
        try:
            if data:
                buffer = data.decode("utf-8")
                print(buffer)
        except:
            print('[Exception]Incorrect json format(main)')
        
        print('Response send')
    else:
        print('empty bytearray data')

#while True:
    #conn, addr = sock.accept()
    #data = conn.recv(1024)
    #if conn
    #else:
    #if not data:
        #break
    #flagCorrect = CheckId(data.decode("utf-8"))
    
    #if data:
        #buffer += data.decode("utf-8")
        #data = None
        #flag = True
    #else:
        #if flag and not data:
            #print('[begin message]')
            #print(buffer)
            #print('[end message]')
            #if CheckId(buffer):
                #print('correct id')
            #else:
                #print('incorrect id')
            #break        
    
    
    #data = conn.recv(1000)
    #if data:
        #buffer += data.decode("utf-8")
        #hexbuffer += binascii.hexlify(data)
    #else:
        #print('[begin message]')
        #print(buffer)
        #print(hexbuffer)
        #print('[end message]')
        #if CheckId(buffer):
            #print('correct id')
        #else:
            #print('incorrect id')#
            
conn.close()
