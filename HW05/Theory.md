# Теория GTK 4 для ДЗ: дерево файлов в GtkTreeView

Документ рассчитан на то, чтобы **без интернета** собрать минимальное приложение на **GTK 4**: окно + прокручиваемое дерево каталога запуска, заполненное **рекурсивно**. Библиотеки — только **GLib** и **GTK** (как в формулировке задания).

**Сборка:** `pkg-config` имя пакета — **`gtk4`** (не `gtk+-3.0`). Готовый каркас см. в [`gtk4_example.c`](gtk4_example.c).

---

## 1. Что вы программируете

**GTK 4** — набор виджетов для UI на C, поверх **GObject** (типы, свойства, сигналы, счётчик ссылок).

| Слой | Роль |
|------|------|
| **Модель** | Данные дерева — обычно **`GtkTreeStore`**. |
| **Представление** | **`GtkTreeView`** + колонки + рендерер (например текст). |
| **Виджеты** | **`GtkApplicationWindow`**, **`GtkScrolledWindow`**, при необходимости **`GtkBox`** / **`GtkCenterBox`**. |

Главный цикл: **`g_application_run`** / **`gtk_application_run`** — блокируется, пока приложение не завершится.

---

## 2. Минимальное приложение (GTK 4)

Идея: **`GtkApplication`**, в колбэке **`activate`** собираете окно, кладёте в него дочерний виджет, вызываете **`gtk_window_present`**.

```c
#include <gtk/gtk.h>

static void on_app_activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Дерево каталога");
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);

    /* 1) Собрать GtkScrolledWindow + GtkTreeView (см. §3).
     * 2) gtk_window_set_child(GTK_WINDOW(window), scroll);
     * 3) Только после этого: */
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv)
{
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.hw05", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
```

Важно для GTK 4:

- **Нет** `gtk_widget_show_all` — окно показывают через **`gtk_window_present(GTK_WINDOW(window))`** после того, как задано содержимое (**`gtk_window_set_child`**).
- Дочерний виджет окна **один**: обычно это **`GtkScrolledWindow`**, внутри которого **`GtkTreeView`**.

Если на старом GLib ругается **`G_APPLICATION_DEFAULT_FLAGS`**, замените на **`G_APPLICATION_FLAGS_NONE`** (см. документацию вашей версии GLib).

Под `-std=c11` и `-Wextra`: неиспользуемые параметры колбэка помечайте **`(void)param;`**.

---

## 3. Склейка окна, прокрутки и дерева (GTK 4)

```c
GtkWidget *scroll = gtk_scrolled_window_new();
gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                               GTK_POLICY_AUTOMATIC,
                               GTK_POLICY_AUTOMATIC);

GtkWidget *tree = make_tree_view_for_cwd(); /* см. ниже */
gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), tree);

gtk_window_set_child(GTK_WINDOW(window), scroll);
gtk_window_present(GTK_WINDOW(window));
```

Отличия от старых учебников (GTK 3):

- **`gtk_scrolled_window_new()`** — **без** аргументов `NULL, NULL`.
- **`gtk_scrolled_window_set_child`** и **`gtk_window_set_child`** вместо **`gtk_container_add`**.
- **`gtk_window_present`** вместо **`gtk_widget_show_all`**.

---

## 4. GtkTreeStore: колонки и типы

`GtkTreeStore` хранит **дерево** строк. При создании задаёте **число колонок и GType каждой**.

| Индекс | Тип | Содержимое |
|--------|-----|------------|
| 0 | `G_TYPE_STRING` | Имя файла/папки |
| 1 | `G_TYPE_STRING` | Полный путь (опционально, для отладки) |

```c
enum { COL_NAME = 0, COL_PATH = 1, NUM_COLS = 2 };

GtkTreeStore *store = gtk_tree_store_new(NUM_COLS,
                                         G_TYPE_STRING,
                                         G_TYPE_STRING);
```

Добавление строки как ребёнка родителя (`parent == NULL` — верхний уровень):

```c
GtkTreeIter iter;
gtk_tree_store_append(store, &iter, parent);
gtk_tree_store_set(store, &iter,
                   COL_NAME, "подпапка",
                   COL_PATH, "/abs/path/подпапка",
                   -1);
```

`GtkTreeIter` **локальный**: не храните его долго; для рекурсивного обхода обычно достаточно передать копию `iter` в следующий вызов как родителя.

---

## 5. GtkTreeView: модель и колонка

```c
GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
g_object_unref(store); /* представление держит ссылку на модель */

GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
        "Имя", renderer, "text", COL_NAME, NULL);
gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

gtk_tree_view_expand_all(GTK_TREE_VIEW(tree));
```

---

## 6. Обход каталогов через GLib (`GDir`)

Логика не зависит от версии GTK: **`g_dir_open`**, **`g_dir_read_name`**, **`g_build_filename`**, **`g_file_test`**.

- Пропускайте **`"."`** и **`".."`**.
- Симлинки на каталоги могут давать повторный обход — для ДЗ часто достаточно простого варианта.

```c
#include <glib.h>
#include <gtk/gtk.h>

static void populate_dir(GtkTreeStore *store, GtkTreeIter *parent, const gchar *dir_path)
{
    GError *err = NULL;
    GDir *dir = g_dir_open(dir_path, 0, &err);

    if (dir == NULL) {
        g_warning("Не удалось открыть каталог %s: %s", dir_path, err->message);
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
```

Старт — **текущий каталог процесса**:

```c
gchar *cwd = g_get_current_dir();
if (cwd != NULL) {
    populate_dir(store, NULL, cwd);
    g_free(cwd);
}
```

Вариант без «фиктивной» строки-корня: сразу **`populate_dir(store, NULL, cwd)`** — верхний уровень = файлы и папки CWD. Вариант с одной строкой-путём в корне — как в [`gtk4_example.c`](gtk4_example.c).

---

## 7. Сортировка имён (опционально)

`g_dir_read_name` порядок не гарантирует. Соберите имена в **`GPtrArray`**, отсортируйте компаратором для указателей на строки:

```c
static gint compare_cstr_ptr(gconstpointer a, gconstpointer b)
{
    const gchar * const *sa = (const gchar * const *)a;
    const gchar * const *sb = (const gchar * const *)b;
    return g_strcmp0(*sa, *sb);
}
```

После второго прохода: **`g_ptr_array_free(names, TRUE)`** если строки выделяли через **`g_strdup`**.

---

## 8. Сборка: `pkg-config`, gcc, Meson

**Одна команда (gcc):**

```bash
gcc -Wall -Wextra -Wpedantic -std=c11 \
  $(pkg-config --cflags gtk4) \
  -o hw05 main.c \
  $(pkg-config --libs gtk4)
```

**Makefile (только gtk4):**

```makefile
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
PKG_CFLAGS := $(shell pkg-config --cflags gtk4)
PKG_LIBS   := $(shell pkg-config --libs gtk4)

hw05: main.c
	$(CC) $(CFLAGS) $(PKG_CFLAGS) -o $@ main.c $(PKG_LIBS)

clean:
	rm -f hw05
```

**Meson** в этой папке: см. **`meson.build`**, команды `meson setup build` и `meson compile -C build`.

---

## 9. Полный каркас в одном файле (GTK 4)

Ниже — логика как в [`gtk4_example.c`](gtk4_example.c): рекурсия, дерево, GTK 4 API для окна и прокрутки.

```c
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
    gtk_window_set_title(GTK_WINDOW(window), "Файлы и каталоги (CWD)");
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

    app = gtk_application_new("com.example.hw05", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
```

---

## 10. GObject: память и ссылки

- **`gtk_tree_view_new_with_model`** увеличивает refcount модели; после привязки часто делают **`g_object_unref(store)`**, если отдельный указатель больше не нужен.
- После **`g_application_run`** у **`GtkApplication`** обычно вызывают **`g_object_unref(app)`**.
- Строки для **`gtk_tree_store_set`** с **`G_TYPE_STRING`** **копируются** в модель — свой **`gchar *`** (например из **`g_build_filename`**) можно **`g_free`** после **`set`**, как в примере с **`full`**.

---

## 11. Сигналы

```c
g_signal_connect(button, "clicked", G_CALLBACK(on_clicked), user_pointer);
```

Для минимального ДЗ дополнительные обработчики не обязательны.

---

## 12. Частые предупреждения компилятора

| Проблема | Как лечить |
|----------|------------|
| Неиспользуемый параметр | `(void)param;` |
| `G_APPLICATION_DEFAULT_FLAGS` на старом GLib | `G_APPLICATION_FLAGS_NONE` или обновить GLib |
| Неверная сигнатура колбэка | сверить с документацией сигнала |
| Нужен каст виджета | `GTK_WINDOW(w)`, `GTK_TREE_VIEW(t)` и т.д. |

---

## 13. Чеклист перед сдачей

- [ ] Запуск из произвольной папки: видно **содержимое CWD**.
- [ ] **Рекурсия** по подкаталогам.
- [ ] **`GtkTreeView`** + **`GtkTreeStore`**.
- [ ] Только **GLib/GTK**.
- [ ] **`-Wall -Wextra -Wpedantic -std=c11`** (deprecated по условию ДЗ можно).

---

## 14. Документация офлайн

Поставьте dev-пакет **GTK 4** и при необходимости откройте **devhelp** или файлы **`gtk-doc`** в системе (`gtk4`, не `gtk+-3.0`).
