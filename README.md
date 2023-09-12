# th19p2vp

a tool for th19 to P2P pvp



before start game, it's recommended to make sure host's and guest's game options is the same( e.g. VS mode selection, keyboard setting...)

after starting game, make the highlight option in the same selection of the same menu( e.g. Story Mode)

the small window in game:



**1.host(P1)**
	if both host and guest supports ipv6(use webs like https://test-ipv4.com to test), then you can click "start as host(ipv6)", or click "start as host(ipv4)"

​	if NAT traversal is not used, then press win+R, enter and run cmd, and enter command "ipconfig/all", host ip is what it shows.(if there's a temporary 

address for ipv6, then use that address). host port is 10800 by default

​	if NAT tranversal is used, then use the address it provides

​	delay: to compensate the delay between 2 players, if feel the game is lagging, then increase it (and stop connection and restart)

​	after click "start as host" button, the capital will be "waiting for 2P" if there's no problem. Then you can wait P2 to connect



**2.guest(P2)**
	hostIP: fill the ip and port of host in. e.g.   "```123.45.67:89```" (ipv4:port) or "```[1145:14:1919:810:abcd:ef01:2345:67]:999```" ([ipv6]:port)

​	if NAT tranversal is used, then use the address it provides

​	then(while host is waiting) click start as guest



after host / guest are both prepared, the capital will be "PVP"

then only host can move, until enterd  VS mode -> human vs human (**not** online vs mode)

then 1/2P can do all operations

if the capital changes to "desync probably", it means the game seems desync (random seeds are wrong), it's needed to go back to player-select menu and wait the patch to resync.(if the patch failed to sync, then click **stop connect** and restart)

it's likely to desync when quitting game stage.



advanced settings:

if host's port 10800 is occupied by other program, then you can change it to another port(the port send to guest needs to change too)

if guest's port 10801 is occupied, then you can change it to another port

if checked "log" checkbox, then click "save log", it will save the log "latest.log" in the game folder for debug.

click show cmd can show cmd (it may cause lag when logging)



** since i only have keyboard, the gamepad control might be buggy(**



updates:

23/09/12
	changed UI, make it easier to use

23/09/11
	make patch try to resync when the game desyncs and is not in game
	changes UI slightly