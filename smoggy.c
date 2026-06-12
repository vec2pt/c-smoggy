/* Acknowledgments:
 * https://curl.se/libcurl/c/getinmemory.html
 * https://www.geeksforgeeks.org/c/cjson-json-file-write-read-modify-in-c/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>
#include <curl/curl.h>

static const char separator[] = ": ";

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t write_cb(char *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if (!ptr) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

int main(void) {
  CURL *curl;
  CURLcode result;
  struct MemoryStruct chunk;
  int rc = EXIT_FAILURE;

  result = curl_global_init(CURL_GLOBAL_ALL);
  if (result != CURLE_OK)
    return (int)result;

  curl = curl_easy_init();
  if (curl == NULL) {
    fprintf(stderr, "curl_easy_perform() failed\n");
    curl_global_cleanup();
    return EXIT_FAILURE;
  }

  chunk.memory = malloc(1);
  chunk.size = 0;

  // clang-format off
  curl_easy_setopt(curl, CURLOPT_URL,
                   "https://air-quality-api.open-meteo.com/v1/air-quality?latitude=50.8643&longitude=21.0905&current=pm10,pm2_5,ozone,nitrogen_dioxide,sulphur_dioxide,carbon_monoxide,european_aqi,us_aqi&forecast_days=1");
  // clang-format on
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

  result = curl_easy_perform(curl);
  if (result != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(result));
    curl_easy_cleanup(curl);
    free(chunk.memory);
    curl_global_cleanup();
    return (int)result;
  }
  // printf("%s\n", chunk.memory);
  // printf("%lu bytes retrieved\n", (unsigned long)chunk.size);

  cJSON *json = cJSON_Parse(chunk.memory);
  if (json == NULL) {
    fprintf(stderr, "cJSON_Parse() failed\n");
    goto cleanup;
  }

  cJSON *current = cJSON_GetObjectItemCaseSensitive(json, "current");
  if (!cJSON_IsObject(current)) {
    fprintf(stderr, "cJSON_GetObjectItemCaseSensitive() failed for current\n");
    goto cleanup;
  }

  cJSON *current_units =
      cJSON_GetObjectItemCaseSensitive(json, "current_units");
  if (!cJSON_IsObject(current_units)) {
    fprintf(stderr,
            "cJSON_GetObjectItemCaseSensitive() failed for current_units\n");
    goto cleanup;
  }

  // printf(
  //     "AQI: %d %s\n",
  //     cJSON_GetObjectItemCaseSensitive(current,
  //     "european_aqi")->valueint,
  //     cJSON_GetObjectItemCaseSensitive(current_units, "european_aqi")
  //         ->valuestring);
  //
  // printf("US AQI: %d %s\n",
  //        cJSON_GetObjectItemCaseSensitive(current, "us_aqi")->valueint,
  //        cJSON_GetObjectItemCaseSensitive(current_units, "us_aqi")
  //            ->valuestring);

  printf("PM10%s%.1f %s\n", separator,
         cJSON_GetObjectItemCaseSensitive(current, "pm10")->valuedouble,
         cJSON_GetObjectItemCaseSensitive(current_units, "pm10")->valuestring);
  printf("PM2.5%s%.1f %s\n", separator,
         cJSON_GetObjectItemCaseSensitive(current, "pm2_5")->valuedouble,
         cJSON_GetObjectItemCaseSensitive(current_units, "pm2_5")->valuestring);
  printf("Ozone 03%s%.1f %s\n", separator,
         cJSON_GetObjectItemCaseSensitive(current, "ozone")->valuedouble,
         cJSON_GetObjectItemCaseSensitive(current_units, "ozone")->valuestring);
  printf("Nitrogen Dioxide NO2%s%.1f %s\n", separator,
         cJSON_GetObjectItemCaseSensitive(current, "nitrogen_dioxide")
             ->valuedouble,
         cJSON_GetObjectItemCaseSensitive(current_units, "nitrogen_dioxide")
             ->valuestring);
  printf(
      "Sulphur Dioxide SO2%s%.1f %s\n", separator,
      cJSON_GetObjectItemCaseSensitive(current, "sulphur_dioxide")->valuedouble,
      cJSON_GetObjectItemCaseSensitive(current_units, "sulphur_dioxide")
          ->valuestring);
  printf(
      "Carbon Monoxide CO%s%.1f %s\n", separator,
      cJSON_GetObjectItemCaseSensitive(current, "carbon_monoxide")->valuedouble,
      cJSON_GetObjectItemCaseSensitive(current_units, "carbon_monoxide")
          ->valuestring);

  rc = EXIT_SUCCESS;

cleanup:
  cJSON_Delete(json);
  curl_easy_cleanup(curl);
  free(chunk.memory);
  curl_global_cleanup();
  return rc;
}
