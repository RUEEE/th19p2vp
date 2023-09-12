# th19p2vp

A tool for th19 P2P pvp

## Installation

Download the zip file from the [releases](https://github.com/RUEEE/th19p2vp/releases) page, and unzip it on top of the original game folder. Please make a backup of the game folder before you do this so you can restore it later.

When playing with others, make sure you are using the same version of this patch.

## Getting Started

Before starting the game, it's recommended to make sure the host's and guest's game options are the same (e.g. VS mode selection, keyboard settings...)

When you start the game, to avoid desyncs, don't move around in the menu, immediately begin the process of connection.

When the game opens, you should see a small white window in the top left corner that allows you to configure the connection. If you don't see it, the patch was not installed properly.

## Connection Configuration

### Host (P1)
If both host and guest have a IPv6 address (use a site like https://test-ipv4.com to test), then you can use "start as host(ipv6)". Otherwise, you should use "start as host(ipv4)".
	
To get your host address to provide to the other player, do the following:

* If no NAT traversal software is being used, then press win+R, type `cmd` and press enter, and enter command "ipconfig /all".
  The host ip is what is shown. (If there's a temporary address for ipv6, then use that address).
* If NAT traversal software is used, then use the address it provides

The default host port is 10080.

Other options:

* delay: to compensate the delay between 2 players, if you feel the game is lagging, then increase it (and stop connection and restart)

After clicking the "start as host" button, the window's title will change to "waiting for 2P" if there's no problems. Then you can wait for P2 to connect.


### Guest (P2)
Obtain the host IP and host port from your host, and fill it into the host IP box:

e.g.   "```123.45.67:89```" (ipv4:port) or "```[1145:14:1919:810:abcd:ef01:2345:67]:999```" ([ipv6]:port)

Note that for IPv6 addresses, the braces `[]` around the address are **required**.

Then click "start as guest".


### Connected
After the connection has been established, the mini-window title will change to "PVP".

After connecting, only the host will be able to move around in the menu, until they enter
the VS mode -> Human vs Human menu (**NOT** Online VS Mode!), after which both players may
choose their shot.

If there is a lag spike or the mini-window title changes to "desync probably", it means a likely desync has been detected (random seeds are wrong).
You should go back to the player-select menu and wait for the patch to try resyncing.

(If the patch still fails to sync, stop the connection, back out to the main menu, and reconnect.)

It's likely to desync when quitting the game stage.

## Advanced Settings

If host's port 10800 is occupied by other program, then you can change it to another port. Make sure to let your guest know.

If guest's port 10801 is occupied, then you can change it to another port.

If you check the "log" checkbox, then click "save log", it will save the log "latest.log" in the game folder for debug.

Click show cmd will show the cmd console (it may cause lag when logging)


## Bugs and Known Problems

** since i only have keyboard, the gamepad control might be buggy(**


## Updates

23/09/12
	changed UI, make it easier to use

23/09/11
	make patch try to resync when the game desyncs and is not in game
	changes UI slightly
	
