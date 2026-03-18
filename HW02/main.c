#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "lib/json/cJSON.h"

#define SAFE_FREE(ptr) do { if (ptr) { free(ptr); (ptr) = NULL; } } while (0)

#define MAX_CITY_NAME_LENGTH 256

#define WEATHER_URL_PATTERN "https://wttr.in/%s?format=j1"
#define WEATHER_REQUEST_TIMEOUT_MS 100000

#define DEBUG 0

typedef struct {
    char *data;
    size_t size;
} response_t;

typedef struct {
    char *description;
    char *wind_directory;
    char *speedKmPerHour;
    char *minTemperature;
    char *maxTemperature;
} weather_t;

static long get_weather_data(response_t *response, const char *city);
static size_t write_data(const void *contents, size_t size, size_t nmemb, void *userp);
static int parse_data(weather_t *weather, const char *json);
static void print_weather(const weather_t *weather, const char *city);

int main(int argc, char *argv[]) {
    int ret = 0;
    weather_t weather = {0};
    response_t response = {0};

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <city>\n", argv[0]);
        ret = 1;
        goto cleanup;
    }
    char *city = argv[1];
    if (DEBUG) printf("City: %s\n", city);

    const long http_code = get_weather_data(&response, city);

    if (http_code == 404) {
        fprintf(stderr, "There is no such city");
        ret = 1;
        goto cleanup;
    }
    if (http_code != 200) {
        fprintf(stderr, "Error getting weather: %ld\n", http_code);
        ret = 1;
        goto cleanup;
    }

    const int err = parse_data(&weather, response.data);
    if (err) {
        fprintf(stderr, "Error parsing weather\n");
        ret = err;
        goto cleanup;
    }
    print_weather(&weather, city);


    cleanup:
    SAFE_FREE(response.data);
    SAFE_FREE(weather.maxTemperature);
    SAFE_FREE(weather.minTemperature);
    SAFE_FREE(weather.speedKmPerHour);
    SAFE_FREE(weather.wind_directory);
    SAFE_FREE(weather.description);

    return ret;
}

static long get_weather_data(response_t *response, const char *city) {
    long http_code = -1;
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "curl_easy_init() failed\n");
        goto curl_cleanup;
    }

    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, WEATHER_REQUEST_TIMEOUT_MS);

    char url[MAX_CITY_NAME_LENGTH];
    snprintf(url, sizeof(url), WEATHER_URL_PATTERN, city);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        goto curl_cleanup;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (DEBUG) printf("Weather response: [%s]\n", response->data);

curl_cleanup:
    curl_easy_cleanup(curl);
    return http_code;
}

#define CHECK_NULL(ptr, ret) do { if (!(ptr)) { ret = 1; goto parse_clean_up; } } while (0)
static int parse_data(weather_t *weather, const char *json) {
    int ret = 0;
    cJSON *root = cJSON_Parse(json);
    if (!root) {
        ret = 1;
        goto parse_clean_up;
    }

    // Читаем нулевой элемент объекта current_condition
    const cJSON *current_condition = cJSON_GetObjectItem(root, "current_condition");
    CHECK_NULL(current_condition, ret);
    const cJSON *current = cJSON_GetArrayItem(current_condition, 0);
    CHECK_NULL(current, ret);

    // Читаем нулевой элемент объекта weatherDesc
    const cJSON *weather_description = cJSON_GetObjectItem(current, "weatherDesc");
    CHECK_NULL(weather_description, ret);
    const cJSON *desc = cJSON_GetArrayItem(weather_description, 0);
    CHECK_NULL(desc, ret);

    // Читаем значение value
    const cJSON *value = cJSON_GetObjectItem(desc, "value");
    CHECK_NULL(value, ret);
    weather->description = strdup(value->valuestring);

    // вытаскиваем направление
    const cJSON *wind_directory = cJSON_GetObjectItem(current, "winddir16Point");
    CHECK_NULL(wind_directory, ret);
    weather->wind_directory = strdup(wind_directory->valuestring);

    // вытаскиваем скорость
    const cJSON *speed = cJSON_GetObjectItem(current, "windspeedKmph");
    CHECK_NULL(speed, ret);
    weather->speedKmPerHour = strdup(speed->valuestring);

    // Диапазон температуры на сегодня
    const cJSON *weather_array = cJSON_GetObjectItem(root, "weather");
    CHECK_NULL(weather_array, ret);
    const cJSON *weather_json = cJSON_GetArrayItem(weather_array, 0);
    CHECK_NULL(weather_json, ret);

    // минимальная
    const cJSON *min_temperature = cJSON_GetObjectItem(weather_json, "mintempC");
    CHECK_NULL(min_temperature, ret);
    weather->minTemperature = strdup(min_temperature->valuestring);

    // максимальная
    const cJSON *max_temperature = cJSON_GetObjectItem(weather_json, "maxtempC");
    CHECK_NULL(max_temperature, ret);
    weather->maxTemperature = strdup(max_temperature->valuestring);

    parse_clean_up:
    cJSON_Delete(root);
    if (ret) {
        fprintf(stderr, "Failed to parse JSON\n");
    }
    return ret;
}

static void print_weather(const weather_t *weather, const char *city) {
    printf("Weather in the city %s for today:\n", city);
    printf("Description: %s\n", weather->description);
    printf("Wind directory: %s\n", weather->wind_directory);
    printf("Speed Km/h: %s\n", weather->speedKmPerHour);
    printf("Temperature C: from %s to %s\n", weather->minTemperature, weather->maxTemperature);
}

static size_t write_data(const void *contents, const size_t size, const size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    response_t *resp = userp;

    char *ptr = realloc(resp->data, resp->size + total + 1);
    if (!ptr) {
        return 0;
    }

    resp->data = ptr;
    memcpy(&resp->data[resp->size], contents, total);
    resp->size += total;

    resp->data[resp->size] = '\0';

    return total;
}

