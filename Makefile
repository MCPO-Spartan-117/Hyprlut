INCLUDES ?= `pkg-config --cflags pixman-1 libdrm hyprland pangocairo libinput libudev wayland-server xkbcommon`
CXXFLAGS += -std=c++2b -Wall -shared -fPIC $(INCLUDES)

all:
	$(CXX) -O3 -flto=auto $(CXXFLAGS) main.cpp -o hyprlut.so
	strip hyprlut.so

debug:
	$(CXX) -Og -g $(CXXFLAGS) main.cpp -o hyprlut.so

load:
	hyprctl plugin load "$(PWD)/hyprlut.so"

unload:
	hyprctl plugin unload "$(PWD)/hyprlut.so"

clean:
	rm -f ./hyprlut.so

.PHONY: all debug load unload clean
