#include <stdio.h>
#include "directory_parser.h"



FileTree *create_new_file_tree(FileTree *file_tree) {
    const bool empty_source_file_tree = !file_tree;
    if (empty_source_file_tree) {
        file_tree = g_new0(FileTree, 1);
        file_tree->name = g_get_current_dir();
    }

    const char *directory = file_tree->name;
    GError *error = NULL;
    GDir *dir = g_dir_open(directory, 0, &error);
    if (dir == NULL) {
        printf("Error opening directory: %s\n", error->message);
        g_clear_error(&error);
        if (empty_source_file_tree) {
            g_free(file_tree->name);
            g_free(file_tree);
        }
       return NULL;
    }

    const char *name;
    while ((name = g_dir_read_name(dir)) != NULL) {
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        char *full_name = g_build_filename(directory, name, NULL);
        FileTree *child = g_new0(FileTree, 1);
        child->name = full_name;
        file_tree->children = g_list_append(file_tree->children, child);

        if (g_file_test(full_name, G_FILE_TEST_IS_DIR)) {
            child->directory = true;
            create_new_file_tree(child);
        }
    }
    g_dir_close(dir);
    return file_tree;
}

static void file_tree_free_node(gpointer data) {
    FileTree *t = data;
    if (t == NULL) {
        return;
    }
    g_list_free_full(t->children, file_tree_free_node);
    t->children = NULL;
    g_free(t->name);
    g_free(t);
}

int destroy_file_tree(FileTree *tree) {
    if (tree) {
        g_list_free_full(tree->children, file_tree_free_node);
        tree->children = NULL;
        g_free(tree->name);
        g_free(tree);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    struct file_tree_t *file_tree = create_new_file_tree(NULL);
    printf("%s\n", file_tree->name);
    return destroy_file_tree(file_tree);
}