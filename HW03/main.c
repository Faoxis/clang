#include <stdio.h>
#include <stdlib.h>

struct node {
    long value;
    struct node *next;
};

struct node *add_element(long value, struct node *next) {
    struct node *n = malloc(sizeof(struct node));
    if (!n) abort();
    n->value = value;
    n->next = next;
    return n;
}

void print_int(long x) {
    printf("%ld ", x);
    fflush(NULL);
}

long p(long x) {
    return x & 1;
}

void m(struct node *list, void (*func)(long)) {
    if (!list) return;
    func(list->value);
    m(list->next, func);
}

struct node *f(struct node *list, struct node *acc, long (*predicate)(long)) {
    if (!list) return acc;
    if (predicate(list->value))
        acc = add_element(list->value, acc);
    return f(list->next, acc, predicate);
}

int main(void) {
    long data[] = {4, 8, 15, 16, 23, 42};
    size_t data_length = sizeof(data) / sizeof(data[0]);

    struct node *list = NULL;
    for (size_t i = data_length; i > 0; i--)
        list = add_element(data[i - 1], list);

    m(list, print_int);
    puts("");

    struct node *filtered = f(list, NULL, p);
    m(filtered, print_int);
    puts("");

    return 0;
}
