
#!/usr/bin/python

import dawnmud_pb2
import sys
from Tkinter import *
import tkMessageBox

zone_list = dawnmud_pb2.ZoneList()
nzone = zone_list.zone.add()

# nzone = dawnmud_pb2.Zone()
nzone.name = "Zone Sample 1"
nzone.lifespan = 50000
nzone.age = 0
nzone.bot = 5
nzone.top = 50000
nzone.reset_mode = dawnmud_pb2.Zone.RESET
nzone.number = 50
nzone.builders.append("Builder 1")
nzone.builders.append("Builder 2")
nzone.builders.append("Builder 3")


tzone = zone_list.zone.add()

# nzone = dawnmud_pb2.Zone()
tzone.name = "Zone Sample 2"
tzone.lifespan = 70000
tzone.age = 0
tzone.bot = 7
tzone.top = 70000
tzone.reset_mode = dawnmud_pb2.Zone.RESET
tzone.number = 70
tzone.builders.append("Builder 4")
tzone.builders.append("Builder 5")
tzone.builders.append("Builder 6")



f = open(sys.argv[1], "wb")
f.write(zone_list.SerializeToString())
f.close()

zone_list2 = dawnmud_pb2.ZoneList()

f2 = open(sys.argv[1], "rb")
zone_list2.ParseFromString(f2.read())
f2.close()

for zone in zone_list2.zone:
    print "Name: ", zone.name
    print "Builders: ", zone.builders
    print "Lifespan: ", zone.lifespan
    print "Age: ", zone.age
    print "Bottom: ", zone.bot
    print "Top: ", zone.top
    print "Reset mode: ", zone.reset_mode
    print "Number: ", zone.number
    print ""
    print ""


def helloCallBack():
    tkMessageBox.showinfo("Hello python", "Hello World")

root = Tk()
text = Text(root)
text.insert(INSERT, "Hello.....")
text.insert(END, "Bye Bye.....")
text.grid(row=1, column=1)

text.tag_add("here", "1.0", "1.4")
text.tag_add("start", "1.8", "1.13")
text.tag_config("here", background="yellow", foreground="blue")
text.tag_config("start", background="black", foreground="green")

B = Button(root, text="Hello", command=helloCallBack)
B.grid(row=1, column=2)

root.mainloop()