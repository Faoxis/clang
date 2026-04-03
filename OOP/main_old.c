/*
 * Минимальный GTK 4: окно и кнопка, цвет фона меняется по нажатию (CSS-классы).
 *
 *   gcc -Wall -Wextra -Wpedantic -std=c11 \
 *     $(pkg-config --cflags gtk4) -o hello-gtk4 main.c \
 *     $(pkg-config --libs gtk4)
 *
 * Старый GLib: замените G_APPLICATION_DEFAULT_FLAGS на G_APPLICATION_FLAGS_NONE.
 */
#include <gtk/gtk.h>

static gboolean showing_color_a = TRUE;

static void apply_app_css(void)
{
    static const char *const css =
        "button.color-a {"
        "  background: #e74c3c;"
        "  color: white;"
        "  min-width: 10em;"
        "  min-height: 3em;"
        "}"
        "button.color-b {"
        "  background: #2980b9;"
        "  color: white;"
        "  min-width: 10em;"
        "  min-height: 3em;"
        "}";

    GtkCssProvider *provider = gtk_css_provider_new();

#if GTK_CHECK_VERSION(4, 12, 0)
    gtk_css_provider_load_from_string(provider, css);
#else
    gtk_css_provider_load_from_data(provider, css, -1);
#endif

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

static void on_button_clicked(GtkButton *button, gpointer user_data)
{
    (void)user_data;

    GtkWidget *w = GTK_WIDGET(button);

    if (showing_color_a) {
        gtk_widget_remove_css_class(w, "color-a");
        gtk_widget_add_css_class(w, "color-b");
    } else {
        gtk_widget_remove_css_class(w, "color-b");
        gtk_widget_add_css_class(w, "color-a");
    }

    showing_color_a = !showing_color_a;
}

static void on_app_activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    apply_app_css();

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Hello GTK 4");
    gtk_window_set_default_size(GTK_WINDOW(window), 420, 220);

    GtkWidget *button = gtk_button_new_with_label("Нажми меня");
    gtk_widget_add_css_class(button, "color-a");
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), NULL);

    GtkWidget *center = gtk_center_box_new();
    gtk_center_box_set_center_widget(GTK_CENTER_BOX(center), button);

    gtk_window_set_child(GTK_WINDOW(window), center);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv)
{
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.hw05.hello", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
