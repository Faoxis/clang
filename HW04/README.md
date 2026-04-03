# HW04 — Сборка в UNIX

**Цель:** получить опыт сборки программ для UNIX-подобных ОС.

## Задание

Необходимо скачать curl последней версии и собрать его в любой UNIX-подобной ОС с поддержкой лишь трёх протоколов: **HTTP**, **HTTPS** и **TELNET**.

## Требования

1. Работа осуществляется в UNIX-подобной ОС (любой дистрибутив Linux, любая BSD-система, MacOS).
2. Скачан и распакован исходный код curl.
3. Сборка сконфигурирована с поддержкой лишь трёх протоколов HTTP, HTTPS и TELNET.
4. Осуществлена сборка (установку в систему осуществлять не требуется и не рекомендуется).
5. Собранный curl запущен с ключом `--version` для подтверждения корректности сборки.

## Решение

Задание выполнено в виде bash-скрипта: [build_curl.sh](build_curl.sh).

Результат проверки:

```
$ ./src/curl --version
curl 8.19.1-DEV (x86_64-pc-linux-gnu) libcurl/8.19.1-DEV OpenSSL/3.0.13 zlib/1.3 zstd/1.5.5
Release-Date: [unreleased]
Protocols: http https telnet
Features: alt-svc AsynchDNS HSTS HTTPS-proxy IPv6 Largefile libz NTLM SSL threadsafe TLS-SRP UnixSockets zstd
```
