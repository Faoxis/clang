#!/bin/bash
set -euo pipefail

echo "=== Скачивание curl ==="
git clone https://github.com/curl/curl

cd curl

echo "=== Конфигурация (только HTTP, HTTPS, TELNET) ==="
autoreconf -fi
./configure \
    --disable-dict \
    --disable-file \
    --disable-ftp \
    --disable-gopher \
    --disable-imap \
    --disable-ldap \
    --disable-ldaps \
    --disable-mqtt \
    --disable-pop3 \
    --disable-rtsp \
    --disable-smb \
    --disable-smtp \
    --disable-tftp \
    --disable-ipfs \
    --disable-websockets \
    --without-librtmp \
    --without-libpsl \
    --without-libssh2 \
    --with-openssl

echo "=== Сборка ==="
make -j"$(nproc)"

echo ""
echo "=== Проверка: curl --version ==="
./src/curl --version

echo "=== Очищаем папку, чтобы иметь возможность запустить снова"
cd ../
rm -rf curl
