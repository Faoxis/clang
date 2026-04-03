#ifndef DIRECTORY_PARSER_H
#define DIRECTORY_PARSER_H

#include <stdbool.h>
#include <glib.h>

typedef struct file_tree_t {
    GList *children;
    bool directory;
    char *name;
} FileTree;

FileTree *create_new_file_tree(FileTree *file_tree);
int destroy_file_tree(FileTree *tree);

#endif
