#include <gtk/gtk.h>
#include "directory/directory_parser.h"

static void on_app_activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Дерево каталогов");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    gtk_window_set_child(GTK_WINDOW(window), /* scroll_or_center_widget */ NULL);
    gtk_window_present(GTK_WINDOW(window));
}


int main(int argc, char** argv)
{
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.something", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}