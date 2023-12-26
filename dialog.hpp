#include <gtk/gtk.h>

class Dialog {
private:
  GtkWidget *window;

public:
  Dialog() : window(nullptr) {
    gtk_init(nullptr, nullptr);
  }

  void show(const char *text) {
    window = gtk_application_window_new(gtk_application_new("com.example.dialog", G_APPLICATION_FLAGS_NONE));
    gtk_window_set_title(GTK_WINDOW(window), "Hello World");
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 100);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);

    GtkWidget *label = gtk_label_new(text);
    const char *css = "label { font-size: 48px; color: white; background-color: black; }";
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1, NULL);

    GtkStyleContext *context = gtk_widget_get_style_context(label);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_container_add(GTK_CONTAINER(window), label);
    gtk_widget_show_all(window);
    gtk_window_present(GTK_WINDOW(window));
    g_timeout_add(500, timeout_callback, this);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_main();
  }

private:
  static gboolean timeout_callback(gpointer user_data) {
    Dialog *dialog = static_cast<Dialog *>(user_data);
    gtk_widget_destroy(dialog->window);
    gtk_main_quit();
    return G_SOURCE_REMOVE;
  }
};