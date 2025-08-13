all:
	$(CXX) -Wall -O3 -flto=auto -shared -fPIC main.cpp -o hyprlut.so `pkg-config --cflags pixman-1 libdrm hyprland pangocairo libinput libudev wayland-server xkbcommon` -std=c++2b
	strip hyprlut.so
debug:
	$(CXX) -Wall -Og -shared -fPIC main.cpp -o hyprlut.so -g `pkg-config --cflags pixman-1 libdrm hyprland pangocairo libinput libudev wayland-server xkbcommon` -std=c++2b
clean:
	rm ./hyprlut.so
