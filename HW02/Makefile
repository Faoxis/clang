CC = cc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
LDFLAGS = -lcurl

run: weather
	./weather Moscow

weather: main.c lib/json/cJSON.c
	$(CC) $(CFLAGS) main.c lib/json/cJSON.c -o weather $(LDFLAGS)

clean:
	rm -f weather

.PHONY: clean
