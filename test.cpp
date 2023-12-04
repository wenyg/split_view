#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/record.h>
#include <X11/extensions/XTest.h>
#include <X11/XKBlib.h>
#include <iostream>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <climits>  // Add this line to include the necessary header


Window getActiveWindow(Display *display) {
    Atom prop = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    Atom type;
    int format;
    unsigned long nitems, bytes_after;
    unsigned char *data;

    int status = XGetWindowProperty(
            display, DefaultRootWindow(display), prop, 0, 1, False,
            XA_WINDOW, &type, &format, &nitems, &bytes_after, &data
    );

    if (status == Success && type == XA_WINDOW && format == 32 && nitems > 0) {
        Window *window = (Window *) data;
        Window result = *window;
        XFree(data);
        return result;
    } else {
        return None;
    }
}

void getWindowGeometry(Display *display, Window window, int &x, int &y, unsigned int &width, unsigned int &height) {
    Atom real_type;
    int real_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop_data;

    Atom net_wm_geometry = XInternAtom(display, "_NET_WM_GEOMETRY", False);

    if (XGetWindowProperty(display, window, net_wm_geometry, 0, 4, False, AnyPropertyType,
                           &real_type, &real_format, &nitems, &bytes_after, &prop_data) == Success) {
        if (nitems == 4) {
            unsigned int *geometry = (unsigned int *) prop_data;
            x = (int) geometry[0];
            y = (int) geometry[1];
            width = geometry[2];
            height = geometry[3];
        }
        XFree(prop_data);
    }
}

int main() {
    Display *display = XOpenDisplay(NULL);

    if (display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    Window active_window = getActiveWindow(display);

    int x, y;
    unsigned int width, height;

    getWindowGeometry(display, active_window, x, y, width, height);

    printf("Window Position: x=%d, y=%d\n", x, y);
    printf("Window Size: width=%u, height=%u\n", width, height);

    XCloseDisplay(display);

    return 0;
}
