# kbi
Simple keyboard layout indicator for system tray. GNU/Linux

Build:

    gcc -O2 -s -lX11 `pkg-config --cflags --libs gtk+-3.0 pangocairo` -o kbi kbi.c
