#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool isJpeg(FILE* f);

typedef struct {
    char **fileNames;
    uint16_t filesCount;
} ZipStruct;

typedef struct {
    ZipStruct *zip;
    bool isJpeg;
} FileStruct;



int main(void) {
    FILE *f = fopen("./test_jpeg_with_zip.jpg", "rb");

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
        ZipStruct zipStruct = {};
        zipStruct.fileNames = malloc(sizeof(char *) * filesCount);
        zipStruct.filesCount = filesCount;
        fileStruct.zip = &zipStruct;

        // central directory link
        fseek(f, 4, SEEK_CUR);
        uint32_t centralDirectoryLink;
        fread(&centralDirectoryLink, sizeof(uint32_t), 1, f);

        // ----- имя файлов ---
        fseek(f, centralDirectoryLink, SEEK_SET);
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

            zipStruct.fileNames[i] = malloc(nameLen + 1);
            memcpy(zipStruct.fileNames[i], fileName, nameLen + 1);

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



// FF D9 - конец jpeg файла
// 0x50 0x4B 0x03 0x04 - признак zip архива


// EOCD
// Байт 0-3:    сигнатура 50 4B 05 06
// ...
// Байт 10-11:  количество файлов        ←  сколько раз читать в цикле
// ...
// Байт 16-19:  смещение Central Directory  ←  куда прыгать через fseek
//
// Из EOCD тебе нужны только два числа: сколько файлов и где лежит Central Directory.


// Central Directory
// Байт 0-3:    Сигнатура (50 4B 01 02)
// Байт 4-5:    Версия, которой создан архив
// Байт 6-7:    Версия, нужная для распаковки
// Байт 8-9:    Битовые флаги (шифрование и т.д.)
// Байт 10-11:  Метод сжатия (0 = без сжатия, 8 = deflate)
// Байт 12-13:  Время модификации файла
// Байт 14-15:  Дата модификации файла
// Байт 16-19:  CRC-32 (контрольная сумма данных)
// Байт 20-23:  Сжатый размер файла
// Байт 24-27:  Несжатый размер файла
// Байт 28-29:  Длина имени файла (N)
// Байт 30-31:  Длина extra field (E)
// Байт 32-33:  Длина комментария к файлу (C)
// Байт 34-35:  Номер диска, на котором начинается файл (легаси)
// Байт 36-37:  Внутренние атрибуты файла
// Байт 38-41:  Внешние атрибуты файла (права доступа и т.д.)
// Байт 42-45:  Смещение Local File Header от начала архива
// ─────────── конец фиксированной части (46 байт) ──────────
// Байт 46:     Имя файла (N байт)
// Байт 46+N:   Extra field (E байт)
// Байт 46+N+E: Комментарий к файлу (C байт)
