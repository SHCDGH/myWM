# My windows manager.
Window manager made by me. It's built for X11 in C.

# Features
- Alt+Enter opens xterm
- Window border featuring close and resize button
- Moveable windows

# How to compile
To compile the WM, you will need:
 - build-essential
 - libx11
 - x11-xserver-utils
 - xterm
 - Xephyr (only if you want to test the WM)

In Debian/Debian-based:
```
sudo apt update
sudo apt install build-essential libx11-dev x11-xserver-utils xterm
sudo apt install xserver-xephyr (this command is optional!)
make clean && make
```
In Arch/Arch-based:
```
sudo pacman -S base-devel libx11 xorg-xprop xterm
sudo pacman -S xorg-server-xephyr (this command is optional!)
make clean && make
```
# How to run/test
To test the WM you need xephyr.
In Debian/Debian-based:
```
sudo apt update
sudo apt install xserver-xephyr
./run.sh
```
In Arch/Arch-based:
```
sudo pacman -S xorg-server-xephyr
./run.sh
```
# To-do list
- [ ] Add window focus
- [ ] Modify the window borders a bit
- [x] Make resize stop being able to make the window a negative size
- [ ] Make the window border dissapear if app is closed internally
- [ ] Add a minimize button
- [ ] Make app launcher
- [ ] Add more key combinations

