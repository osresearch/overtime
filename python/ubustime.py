#!/usr/bin/env python3
import client
import gvbparser

# haarlemmerplein and # ponts Buiksloterweg, Distelweg, NDSM
halts = [ "02133" ] #, "02114", "09906", "09901", "09902" ]

ws = client.WebsocketClient("wss://maps-wss.gvb.nl/")
msg = ws.recv()
print(msg)

print("connected: adding stops")
for halt in halts:
	ws.send('[5,"/stops/%s"]' % (halt))
#ws.send(b'[5,"/stops/02133"]\r\n')


line = ''
while ws.open:
	# we want to receive a line at a time
	msg = ws.recv()
	if msg is None:
		continue

	gvbparser.parse(msg)

print("EXIT")
