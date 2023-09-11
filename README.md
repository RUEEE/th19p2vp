# th19p2vp

a tool for th19 to P2P pvp



before start game, it's recommended to make sure host's and guest's game options is the same( e.g. VS mode selection, keyboard setting...)
after starting game, make the highlight option in the same selection of the same menu( e.g. Story Mode)

the small window in game:
**1.host(P1)**
	HostIP: fill host's ip (both ipv4/6 is ok, but make sure 2 computers both support ipv6 if use ipv6 (use webs like https://test-ipv4.com to test))
	delay: to compensate the delay between computers, if feel the game is lagging, then increase it (and stop connection and restart)
	port: don't need to change for host usually (unless port 9961/9962 is occupied)
	after filled all blanks, click "start as host", then the small window's capital will be "waiting for P2"

**2.guest(P2)**
	HostIP / Host port: fill host's ip/port (if used NAT penetration , then fill in the NAT penetration's address)
	delay: guest don't need to change, it will automatically synchronize with host after start
	then (after host clicks "start as host" and capital changes), click "start as guest"

after host / guest are both prepared, the capital will be "PVP"
then only host can move, until enterd  VS mode -> human vs human (**not** online vs mode)
then 1/2P can do all operations

if the capital changes to "desync probably", it means the game seems desync (random seeds are wrong), it's needed to go back to player-select menu and wait the patch to resync.(if the patch failed to sync, then click **stop connect** and restart)

it's likely to desync when quitting game.

if checked "log" checkbox, then click "save log", it will save the log "latest.log" in the game folder for debug.
click show cmd can show cmd (it may cause lag when logging)

*** since i only have keyboard, the gamepad control might be buggy(



updates:

23/9/11

make patch try to resync when the game desyncs and is not in game

changes UI slightly