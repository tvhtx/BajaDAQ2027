import serial

"""
STEPS BEFORE RUNNING CODE(On a windows 11 computer):

0. Run the Arduino code to activate the ESP32 bluetooth
1. Go to settings -> Bluetooth & devices
2. Click on "View more devices"
3. Scroll down until you find "More Bluetooth settings" and click on it
4. Click on COM Ports then "Add..."
5. Select "Outgoing", then click on "Browse..."
6. You should see the name you gave the ESP32 in the Arduino code. Click on it and press "OK"
7. Press "OK" again and now you should see the ESP32 listed as well as its COM port
8. Enter in the COM port in the port variable below and run this code

"""


port = "COM4" # change to your COM port
baud = 115200 # must be same baud rate as you set in Arduino code

with serial.Serial(port, baud, timeout=1) as bt:
    print(f"Connected to {port}")
    
    while True:
        line = bt.readline()
        if line:
            print(line.decode("utf-8").strip())