from serial import Serial
import os

current_path = os.path.dirname(os.path.realpath(__file__))

ser = Serial("COM5", 115200)
f = open(os.path.join(current_path, "data.json"), 'w+')

data = ''
line = 0

while line < 51 * 50 * 20:
    com_input = ser.readline()
    if com_input:
        s = com_input.decode()
        data += s

        print(line / 20 / 50)
        line += 1
    else:
        break

f.write(data)
f.close()
ser.close()
