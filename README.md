# My windows manager.
Window manager made by me. It's built for X11 in C.

# Key combinations
Alt+Tab opens xterm.

# How to compile
To compile the WM, you will need:
 - build-essential
 - libx11-dev
 - x11-xserver-utils
 - xterm
 - xserver-xephyr (only if you want to test the WM)

In Debian/Debian-based:
```
sudo apt update
sudo apt install build-essential libx11-dev x11-xserver-utils xterm
sudo apt install xserver-xephyr (this command is optional!) 
```
In Arch/Arrch-based:
```
sudo pacman -S base-devel libx11 xorg-xprop xterm
sudo pacman -S xorg-server-xephyr (this command is optional!)
```
