/*
 * Минимальный пример для HW05 под GTK 4: рекурсивное дерево каталога в GtkTreeView.
 * Сборка:
 *   gcc -Wall -Wextra -Wpedantic -std=c11 \
 *     $(pkg-config --cflags gtk4) -o gtk4_example gtk4_example.c \
 *     $(pkg-config --libs gtk4)
 *
 * Если g_application_new не принимает G_APPLICATION_DEFAULT_FLAGS — замените на
 * G_APPLICATION_FLAGS_NONE (старый GLib).
 */
#include <gtk/gtk.h>

enum { COL_NAME = 0, COL_PATH = 1, NUM_COLS = 2 };

static void populate_dir(GtkTreeStore *store, GtkTreeIter *parent, const gchar *dir_path)
{
    GError *err = NULL;
    GDir *dir = g_dir_open(dir_path, 0, &err);

    if (dir == NULL) {
        g_warning("%s", err->message);
        g_clear_error(&err);
        return;
    }

    const gchar *name;
    while ((name = g_dir_read_name(dir)) != NULL) {
        if (g_strcmp0(name, ".") == 0 || g_strcmp0(name, "..") == 0) {
            continue;
        }

        gchar *full = g_build_filename(dir_path, name, NULL);

        GtkTreeIter iter;
        gtk_tree_store_append(store, &iter, parent);
        gtk_tree_store_set(store, &iter,
                           COL_NAME, name,
                           COL_PATH, full,
                           -1);

        if (g_file_test(full, G_FILE_TEST_IS_DIR)) {
            populate_dir(store, &iter, full);
        }

        g_free(full);
    }

    g_dir_close(dir);
}

static GtkWidget *make_tree_view_for_cwd(void)
{
    GtkTreeStore *store = gtk_tree_store_new(NUM_COLS,
                                             G_TYPE_STRING,
                                             G_TYPE_STRING);

    gchar *cwd = g_get_current_dir();
    if (cwd != NULL) {
        populate_dir(store, NULL, cwd);
        g_free(cwd);
    }

    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
            "Имя", renderer, "text", COL_NAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    gtk_tree_view_expand_all(GTK_TREE_VIEW(tree));
    return tree;
}

static void on_app_activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Файлы и каталоги (GTK 4, CWD)");
    gtk_window_set_default_size(GTK_WINDOW(window), 720, 480);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    GtkWidget *tree = make_tree_view_for_cwd();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), tree);

    gtk_window_set_child(GTK_WINDOW(window), scroll);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv)
{
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.hw05.gtk4", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
