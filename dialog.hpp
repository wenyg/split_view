#include <gtk/gtk.h>

class Dialog {
public:
  static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Hello World");

    // 确保窗口一直显示在其他窗口的前面
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);

    // 设置窗口默认大小
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 100);

    // 隐藏窗口的菜单栏
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);

    // 设置窗口弹出的位置为屏幕中间下方
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);

    // 设置窗口不在任务栏中显示
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    const char *text = static_cast<const char *>(user_data);

    GtkWidget *label = gtk_label_new(text);

    // 设置标签的字体样式、颜色和背景色
    const char *css =
        "label { font-size: 48px; color: white; background-color: black; }";
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1, NULL);

    GtkStyleContext *context = gtk_widget_get_style_context(label);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_container_add(GTK_CONTAINER(window), label);

    gtk_widget_show_all(window);

    // 确保窗口在最上层
    gtk_window_present(GTK_WINDOW(window));

    // 安排一个定时器，在500ms后关闭窗口
    g_timeout_add(500, timeout_callback, window);
  }

  static gboolean timeout_callback(gpointer user_data) {
    GtkWidget *window = GTK_WIDGET(user_data);
    gtk_widget_destroy(window);
    return G_SOURCE_REMOVE;
  }

  static void show(const char *text) {
    GtkApplication *app;
    int argc = 0;
    char **argv = NULL;
    int status;

    app = gtk_application_new("com.example.dialog", G_APPLICATION_FLAGS_NONE);

    g_signal_connect(app, "activate", G_CALLBACK(activate), (gpointer)text);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
  }
};