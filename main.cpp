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

typedef struct _XCape_t {
    Display *data_conn;
    Display *ctrl_conn;
    XRecordContext record_ctx;
    pthread_t sigwait_thread;
    sigset_t sigset;
    Bool foreground;
    Bool debug;
    struct timeval timeout;
} XCape_t;

void *sig_handler(void *user_data);

void intercept(XPointer user_data, XRecordInterceptData *data);


bool getWorkAreaSize(int &width, int &height) {
    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        fprintf(stderr, "Unable to open X display\n");
        return false;
    }

    Window root = DefaultRootWindow(display);
    Atom atom = XInternAtom(display, "_NET_WORKAREA", False);

    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char *prop_data;

    int status = XGetWindowProperty(
            display,
            root,
            atom,
            0,
            LONG_MAX,
            False,
            XA_CARDINAL,
            &actual_type,
            &actual_format,
            &nitems,
            &bytes_after,
            &prop_data
    );

    if (status == Success && actual_format == 32 && nitems >= 4) {
        long *workarea = reinterpret_cast<long *>(prop_data);
        width = workarea[2];
        height = workarea[3];

        XFree(prop_data);
        XCloseDisplay(display);
        return true;
    } else {
        fprintf(stderr, "Failed to get _NET_WORKAREA property\n");
        XCloseDisplay(display);
        return false;
    }
}

int main(int argc, char **argv) {
    XCape_t *self = (XCape_t *) malloc(sizeof(XCape_t));

    int dummy, ch;

    XRecordRange *rec_range = XRecordAllocRange();
    XRecordClientSpec client_spec = XRecordAllClients;

    self->foreground = False;
    self->debug = False;
    self->timeout.tv_sec = 0;
    self->timeout.tv_usec = 500000;

    rec_range->device_events.first = KeyPress;
    rec_range->device_events.last = ButtonRelease;

    while ((ch = getopt(argc, argv, "dfe:t:")) != -1) {
        switch (ch) {
            case 'd':
                self->debug = True;
                /* imply -f (no break) */
            case 'f':
                self->foreground = True;
                break;
            case 't': {
                int ms = atoi(optarg);
                if (ms > 0) {
                    self->timeout.tv_sec = ms / 1000;
                    self->timeout.tv_usec = (ms % 1000) * 1000;
                } else {
                    fprintf(stderr, "Invalid argument for '-t': %s.\n", optarg);
//                    print_usage (argv[0]);
                    return EXIT_FAILURE;
                }
            }
                break;
            default:
//                print_usage (argv[0]);
                return EXIT_SUCCESS;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "Not a command line option: '%s'\n", argv[optind]);
//        print_usage (argv[0]);
        return EXIT_SUCCESS;
    }

    if (!XInitThreads()) {
        fprintf(stderr, "Failed to initialize threads.\n");
        exit(EXIT_FAILURE);
    }

    self->data_conn = XOpenDisplay(NULL);
    self->ctrl_conn = XOpenDisplay(NULL);

    if (!self->data_conn || !self->ctrl_conn) {
        fprintf(stderr, "Unable to connect to X11 display. Is $DISPLAY set?\n");
        exit(EXIT_FAILURE);
    }
    if (!XQueryExtension(self->ctrl_conn, "XTEST", &dummy, &dummy, &dummy)) {
        fprintf(stderr, "Xtst extension missing\n");
        exit(EXIT_FAILURE);
    }
    if (!XRecordQueryVersion(self->ctrl_conn, &dummy, &dummy)) {
        fprintf(stderr, "Failed to obtain xrecord version\n");
        exit(EXIT_FAILURE);
    }
    if (!XkbQueryExtension(self->ctrl_conn, &dummy, &dummy, &dummy, &dummy, &dummy)) {
        fprintf(stderr, "Failed to obtain xkb version\n");
        exit(EXIT_FAILURE);
    }

    if (self->foreground != True)
        daemon(0, 0);

    sigemptyset(&self->sigset);
    sigaddset(&self->sigset, SIGINT);
    sigaddset(&self->sigset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &self->sigset, NULL);

    pthread_create(&self->sigwait_thread, NULL, sig_handler, self);

    self->record_ctx = XRecordCreateContext(self->ctrl_conn, 0, &client_spec, 1, &rec_range, 1);

    if (self->record_ctx == 0) {
        fprintf(stderr, "Failed to create xrecord context\n");
        exit(EXIT_FAILURE);
    }

    XSync(self->ctrl_conn, False);

    if (!XRecordEnableContext(self->data_conn, self->record_ctx, intercept, (XPointer) self->ctrl_conn)) {
        fprintf(stderr, "Failed to enable xrecord context\n");
        exit(EXIT_FAILURE);
    }

    pthread_join(self->sigwait_thread, NULL);

    if (!XRecordFreeContext(self->ctrl_conn, self->record_ctx)) {
        fprintf(stderr, "Failed to free xrecord context\n");
    }

    if (self->debug) fprintf(stdout, "main exiting\n");

    XFree(rec_range);

    XCloseDisplay(self->ctrl_conn);
    XCloseDisplay(self->data_conn);

    free(self);
    return EXIT_SUCCESS;
}


void *sig_handler(void *user_data) {
    XCape_t *self = (XCape_t *) user_data;
    int sig;

    if (self->debug) fprintf(stdout, "sig_handler running...\n");

    sigwait(&self->sigset, &sig);

    if (self->debug) fprintf(stdout, "Caught signal %d!\n", sig);

    XLockDisplay(self->ctrl_conn);

    if (!XRecordDisableContext(self->ctrl_conn, self->record_ctx)) {
        fprintf(stderr, "Failed to disable xrecord context\n");
        exit(EXIT_FAILURE);
    }

    XSync(self->ctrl_conn, False);

    XUnlockDisplay(self->ctrl_conn);

    if (self->debug) fprintf(stdout, "sig_handler exiting...\n");

    return NULL;
}

bool global_left_ctrl_pressed = false;
bool global_left_alt_pressed = false;
bool global_left_shift_pressed = false;
bool global_left_super_pressed = false;

void resizeAndMoveWindow(Display *display, Window window, float width_percent, float height_percent, float x_percent,
                         float y_percent) {
    XWindowChanges changes;
    int width, height;
    getWorkAreaSize(width, height);
    int display_width = DisplayWidth(display, DefaultScreen(display));
    int menu_bar_width = int(display_width) - width;
    changes.width = width * width_percent;
    changes.height = height * height_percent;
    changes.x = width * x_percent + menu_bar_width;
    changes.y = height * y_percent;
    XConfigureWindow(display, window, CWX | CWY | CWWidth | CWHeight, &changes);
    XSync(display, False);
}

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

// 添加用于打印窗口信息的函数
void printWindowInfo(Display *display, Window window) {
    XWindowAttributes windowAttributes;
    if (XGetWindowAttributes(display, window, &windowAttributes)) {
        fprintf(stdout, "Window Attributes:\n");
        fprintf(stdout, "  X: %d\n", windowAttributes.x);
        fprintf(stdout, "  Y: %d\n", windowAttributes.y);
        fprintf(stdout, "  Width: %d\n", windowAttributes.width);
        fprintf(stdout, "  Height: %d\n\n", windowAttributes.height);
    } else {
        fprintf(stderr, "Failed to get window attributes.\n");
    }
}

void getWindowGeometry(Display *display, Window window, unsigned int &x, unsigned int &y, unsigned int &width,
                       unsigned int &height) {
    Atom real_type;
    int real_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop_data;

    Atom net_wm_geometry = XInternAtom(display, "_NET_WM_GEOMETRY", False);

    if (XGetWindowProperty(display, window, net_wm_geometry, 0, 4, False, AnyPropertyType,
                           &real_type, &real_format, &nitems, &bytes_after, &prop_data) == Success) {
        if (nitems == 4) {
            auto *geometry = (unsigned int *) prop_data;
            x = geometry[0];
            y = geometry[1];
            width = geometry[2];
            height = geometry[3];
        }
        XFree(prop_data);
    }
}

// 函数用于获取当前窗口信息并保存到结构体中
void getCurrentWindowInfo(Display *display, Window window, XWindowChanges *windowInfo) {
    XWindowAttributes attr;
    XGetWindowAttributes(display, window, &attr);
    Window child;
    XTranslateCoordinates(display, window, DefaultRootWindow(display), 0, 0, &(attr.x), &(attr.y), &child);

    windowInfo->x = attr.x;
    windowInfo->y = attr.y;
    windowInfo->width = attr.width;
    windowInfo->height = attr.height;
}

void intercept(XPointer user_data, XRecordInterceptData *data) {
    auto *ctrl_conn = (Display * )(user_data);
    XLockDisplay(ctrl_conn);

    if (data->category == XRecordFromServer) {
        int key_event = data->data[0];
        KeyCode key_code = data->data[1];

        if (key_code == 37) {
            global_left_ctrl_pressed = (key_event == 2);
        } else if (key_code == 64) {
            global_left_alt_pressed = (key_event == 2);
        } else if (key_code == 133) {
            global_left_super_pressed = (key_event == 2);
        } else if (key_code == 50) {
            global_left_shift_pressed = (key_event == 2);
        }

        if (global_left_ctrl_pressed && global_left_alt_pressed && key_event == 2) {
            auto active_window = getActiveWindow(ctrl_conn);
            if (key_code == 87 ) { // 小键盘 1
                resizeAndMoveWindow(ctrl_conn, active_window, 0.3333, 1, 0, 0);
            } else if (key_code == 88 ) { // 小键盘 2
                resizeAndMoveWindow(ctrl_conn, active_window, 0.3333, 1, 0.3333, 0);
            } else if (key_code == 89) { // 小键盘 3
                resizeAndMoveWindow(ctrl_conn, active_window, 0.3333, 1, 0.6666, 0);
            } else if (key_code == 83 ) { // 小键盘 4
                resizeAndMoveWindow(ctrl_conn, active_window, 0.6666, 1, 0, 0);
            } else if (key_code == 85 ) { // 小键盘 6
                resizeAndMoveWindow(ctrl_conn, getActiveWindow(ctrl_conn), 2.0 / 3, 1, 1.0 / 3, 0);
            } else if (key_code == 113) { // 左方向键
                resizeAndMoveWindow(ctrl_conn, active_window, 0.5, 1, 0, 0);
            } else if (key_code == 114) { // 右方向键
                resizeAndMoveWindow(ctrl_conn, active_window, 0.5, 1, 0.5, 0);
            }else if (key_code == 36) { // Enter
                static bool is_full_screen = false;
                static XWindowChanges store_changes;
                if (is_full_screen) {
                    // 如果是全屏状态，恢复窗口到之前保存的位置大小信息
                    XConfigureWindow(ctrl_conn, active_window, CWX | CWY | CWWidth | CWHeight, &store_changes);
                    printWindowInfo(ctrl_conn, active_window);
                } else {
                    // 如果不是全屏状态，保存当前窗口位置大小信息，并将窗口设置为全屏
                    getCurrentWindowInfo(ctrl_conn, active_window, &store_changes);
                    printWindowInfo(ctrl_conn, active_window);
                    resizeAndMoveWindow(ctrl_conn, active_window, 1, 1, 0, 0);
                }
                is_full_screen = !is_full_screen;
            }
        }

        if (global_left_ctrl_pressed && global_left_shift_pressed && key_event == 2) {
            if (key_code == 40) {   // D 
                fprintf(stdout, "/usr/bin/gsettings set org.gnome.system.proxy mode 'auto'\n");
                system("/usr/bin/gsettings set org.gnome.system.proxy mode 'auto'");
            }  else if (key_code == 42) { // G 
                fprintf(stdout, "/usr/bin/gsettings set org.gnome.system.proxy mode 'manual'\n");
                system("/usr/bin/gsettings set org.gnome.system.proxy mode 'manual'");
            }  
        }

        if (key_event == 2) {
            fprintf(stdout, "Intercepted key code %d\n", key_code);
        }
    }

    XUnlockDisplay(ctrl_conn);
    XRecordFreeData(data);
}


