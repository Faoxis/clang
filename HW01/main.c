#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define SAFE_FREE(ptr) do { free(ptr); (ptr) = NULL; } while (0)

bool is_jpeg(FILE* f);
long get_zip_start_offset(FILE* f, bool is_jpeg);

typedef struct {
    char **file_names;
    uint16_t files_count;
} zip_struct_t;

typedef struct {
    zip_struct_t *zip;
    bool is_jpeg;
} file_struct_t;



int main(int argc, char *argv[]) {
    int ret = 0;
    FILE *f = NULL;
    file_struct_t file_struct = {0};
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        ret = 1;
        goto cleanup;
    }

    f = fopen(argv[1], "rb");
    if (f == NULL) {
        perror("fopen");
        ret = 1;
        goto cleanup;
    }

    // Проверка файл на jpeg
    file_struct.is_jpeg = is_jpeg(f);

    // Делаем предположение, что в конце файла zip
    fseek(f, -22, SEEK_END);
    uint8_t zip_eocd[] = { 0x50, 0x4B, 0x05, 0x06 };
    bool zip = true;
    for (int i = 0; i < sizeof(zip_eocd); i++) {
        u_int8_t read_byte;
        fread(&read_byte, sizeof(u_int8_t), 1, f);
        if (zip_eocd[i] != read_byte) {
            zip = false;
            break;
        }
    }

    if (zip) {
        // count of files
        fseek(f, 6, SEEK_CUR);
        uint16_t files_count;
        fread(&files_count, sizeof(uint16_t), 1, f);
        zip_struct_t *zip_struct = malloc(sizeof(zip_struct_t));
        zip_struct->file_names = calloc(files_count, sizeof(char *));
        zip_struct->files_count = files_count;
        file_struct.zip = zip_struct;

        // central directory link (в EOCD — от начала архива, не файла)
        fseek(f, 4, SEEK_CUR);
        uint32_t central_directory_link;
        fread(&central_directory_link, sizeof(uint32_t), 1, f);

        long zip_start = get_zip_start_offset(f, file_struct.is_jpeg);

        // ----- имя файлов ---
        fseek(f, zip_start + (long)central_directory_link, SEEK_SET);
        for (int i = 0; i < files_count; i++) {
            // Переход к center directory
            uint8_t center_dir_id[] = { 0x50, 0x4B, 0x01, 0x02 };
            for (int i = 0; i < sizeof(center_dir_id); i++) {
                uint8_t read_byte;
                fread(&read_byte, sizeof(uint8_t), 1, f);
                if (center_dir_id[i] != read_byte) {
                    printf("Error parsing central directory");
                    ret = 1;
                    goto cleanup;
                }
            }

            // Узнаем длину имени
            fseek(f, 24, SEEK_CUR);
            uint16_t name_len, extra_len, comment_len;
            fread(&name_len, sizeof(uint16_t), 1, f);
            fread(&extra_len, sizeof(uint16_t), 1, f);
            fread(&comment_len, sizeof(uint16_t), 1, f);

            // Читаем имя файла
            fseek(f, 12, SEEK_CUR);
            char file_name[name_len + 1];
            fread(file_name, sizeof(char), name_len, f);
            file_name[name_len] = '\0';

            zip_struct->file_names[i] = malloc(name_len + 1);
            memcpy(zip_struct->file_names[i], file_name, name_len + 1);

            fseek(f, extra_len + comment_len, SEEK_CUR);
        }
    }

    // Печатаем результат
    if (file_struct.is_jpeg) {
        printf("Файл содержит jpeg контент\n");
    } else {
        printf("Файл не содержит jpeg контента\n");
    }
    if (!file_struct.zip) {
        printf("Файл не содержит zip контента\n");
    } else {
        printf("В файле есть контент с zip архивом\n");
        printf("Всего в архиве %d файлов:\n", file_struct.zip->files_count);
        for (int i = 0; i < file_struct.zip->files_count; i++) {
            printf("%s\n", file_struct.zip->file_names[i]);
        }
    }

    cleanup:
        if (file_struct.zip) {
            for (int i = 0; i < file_struct.zip->files_count; i++) {
                SAFE_FREE(file_struct.zip->file_names[i]);
            }
            SAFE_FREE(file_struct.zip->file_names);
            SAFE_FREE(file_struct.zip);
        }
        if (f) {
            fclose(f);
            f = NULL;
        }
        return ret;
}

bool is_jpeg(FILE* f) {
    uint8_t first_byte, second_byte;
    fread(&first_byte, sizeof(uint8_t), 1, f);
    fread(&second_byte, sizeof(uint8_t), 1, f);

    return first_byte == 0xFF && second_byte == 0xD8;
}

long get_zip_start_offset(FILE* f, bool is_jpeg) {
    if (!is_jpeg) {
        return 0;
    }

    rewind(f);
    int c;
    long first_byte_after_jpeg = 0;
    while ((c = fgetc(f)) != EOF) {
        if (c == 0xFF) {
            int c2 = fgetc(f);
            if (c2 == EOF) {
                break;
            }
            if (c2 == 0xD9) {
                first_byte_after_jpeg = ftell(f); /* первый байт после JPEG (EOI) */
            }
        }
    }
    return first_byte_after_jpeg;
}
