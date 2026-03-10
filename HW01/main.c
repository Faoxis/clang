#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool isJpeg(FILE* f);
long getZipStartOffset(FILE* f, bool isJpeg);

typedef struct {
    char **fileNames;
    uint16_t filesCount;
} ZipStruct;

typedef struct {
    ZipStruct *zip;
    bool isJpeg;
} FileStruct;



int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (f == NULL) {
        perror("fopen");
        return 1;
    }

    FileStruct fileStruct = {};

    // Проверка файл на jpeg
    fileStruct.isJpeg = isJpeg(f);

    // Делаем предположение, что в конце файла zip
    fseek(f, -22, SEEK_END);
    uint8_t zipEOCD[] = { 0x50, 0x4B, 0x05, 0x06 };
    bool zip = true;
    for (int i = 0; i < sizeof(zipEOCD); i++) {
        u_int8_t readByte;
        fread(&readByte, sizeof(u_int8_t), 1, f);
        if (zipEOCD[i] != readByte) {
            zip = false;
            break;
        }
    }

    if (zip) {
        // count of files
        fseek(f, 6, SEEK_CUR);
        uint16_t filesCount;
        fread(&filesCount, sizeof(uint16_t), 1, f);
        ZipStruct *zipStruct = malloc(sizeof(ZipStruct));
        zipStruct->fileNames = malloc(sizeof(char *) * filesCount);
        zipStruct->filesCount = filesCount;
        fileStruct.zip = zipStruct;

        // central directory link (в EOCD — от начала архива, не файла)
        fseek(f, 4, SEEK_CUR);
        uint32_t centralDirectoryLink;
        fread(&centralDirectoryLink, sizeof(uint32_t), 1, f);

        long zipStart = getZipStartOffset(f, fileStruct.isJpeg);

        // ----- имя файлов ---
        fseek(f, zipStart + (long)centralDirectoryLink, SEEK_SET);
        for (int i = 0; i < filesCount; i++) {
            // Переход к center directory
            uint8_t centerDirectoryIdentifier[] = { 0x50, 0x4B, 0x01, 0x02 };
            for (int i = 0; i < sizeof(centerDirectoryIdentifier); i++) {
                uint8_t readByte;
                fread(&readByte, sizeof(uint8_t), 1, f);
                if (centerDirectoryIdentifier[i] != readByte) {
                    printf("Error parsing central directory");
                    return 1;
                }
            }

            // Узнаем длину имени
            fseek(f, 24, SEEK_CUR);
            uint16_t nameLen, extraLen, commentLen;
            fread(&nameLen, sizeof(uint16_t), 1, f);
            fread(&extraLen, sizeof(uint16_t), 1, f);
            fread(&commentLen, sizeof(uint16_t), 1, f);

            // Читаем имя файла
            fseek(f, 12, SEEK_CUR);
            char fileName[nameLen + 1];
            fread(fileName, sizeof(char), nameLen, f);
            fileName[nameLen] = '\0';

            zipStruct->fileNames[i] = malloc(nameLen + 1);
            memcpy(zipStruct->fileNames[i], fileName, nameLen + 1);

            fseek(f, extraLen + commentLen, SEEK_CUR);
        }
    }

    // Печатаем результат
    if (fileStruct.isJpeg) {
        printf("Файл содержит jpeg контент\n");
    } else {
        printf("Файл не содержит jpeg контента\n");
    }
    if (!fileStruct.zip) {
        printf("Файл не содержит zip контента\n");
    } else {
        printf("В файле есть контент с zip архивом\n");
        printf("Всего в архиве %d файлов:\n", fileStruct.zip->filesCount);
        for (int i = 0; i < fileStruct.zip->filesCount; i++) {
            printf("%s\n", fileStruct.zip->fileNames[i]);
            free(fileStruct.zip->fileNames[i]);
        }
        free(fileStruct.zip->fileNames);
        free(fileStruct.zip);
    }

    fclose(f);
    return 0;
}

bool isJpeg(FILE* f) {
    uint8_t firstByteInHeader, secondByteInHeader;
    fread(&firstByteInHeader, sizeof(uint8_t), 1, f);
    fread(&secondByteInHeader, sizeof(uint8_t), 1, f);

    if (firstByteInHeader == 0xFF && secondByteInHeader == 0xD8) {
        return true;
    }
    return false;
}

long getZipStartOffset(FILE* f, bool isJpeg) {
    if (!isJpeg) {
        return 0;
    }
    rewind(f);
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == 0xFF) {
            int c2 = fgetc(f);
            if (c2 == EOF) {
                break;
            }
            if (c2 == 0xD9) {
                return ftell(f); /* первый байт после JPEG (EOI) */
            }
        }
    }
    return 0;
}
