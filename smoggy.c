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

struct SmoggyData {
  char *city_name;
  char *city_country_code;
  float city_latitude;
  float city_longitude;

  int european_aqi;
  int us_aqi;
  float pm10;
  float pm2_5;
  float ozone;
  float nitrogen_dioxide;
  float sulphur_dioxide;
  float carbon_monoxide;

  char *european_aqi_unit;
  char *us_aqi_unit;
  char *pm10_unit;
  char *pm2_5_unit;
  char *ozone_unit;
  char *nitrogen_dioxide_unit;
  char *sulphur_dioxide_unit;
  char *carbon_monoxide_unit;
};

struct SmoggyData *smoggy_init() {
  struct SmoggyData *smoggydata = malloc(sizeof(*smoggydata));
  smoggydata->city_name = NULL;
  smoggydata->city_country_code = NULL;
  smoggydata->city_latitude = 0;
  smoggydata->city_longitude = 0;

  smoggydata->european_aqi = 0;
  smoggydata->us_aqi = 0;
  smoggydata->pm10 = 0.0;
  smoggydata->pm2_5 = 0.0;
  smoggydata->ozone = 0.0;
  smoggydata->nitrogen_dioxide = 0.0;
  smoggydata->sulphur_dioxide = 0.0;
  smoggydata->carbon_monoxide = 0.0;

  smoggydata->european_aqi_unit = NULL;
  smoggydata->us_aqi_unit = NULL;
  smoggydata->pm10_unit = NULL;
  smoggydata->pm2_5_unit = NULL;
  smoggydata->ozone_unit = NULL;
  smoggydata->nitrogen_dioxide_unit = NULL;
  smoggydata->sulphur_dioxide_unit = NULL;
  smoggydata->carbon_monoxide_unit = NULL;

  return smoggydata;
}

void smoggy_cleanup(struct SmoggyData *smoggydata) {
  free(smoggydata->city_name);
  free(smoggydata->city_country_code);

  free(smoggydata->european_aqi_unit);
  free(smoggydata->us_aqi_unit);
  free(smoggydata->pm10_unit);
  free(smoggydata->pm2_5_unit);
  free(smoggydata->ozone_unit);
  free(smoggydata->nitrogen_dioxide_unit);
  free(smoggydata->sulphur_dioxide_unit);
  free(smoggydata->carbon_monoxide_unit);

  free(smoggydata);
}

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

CURLcode get_curl_data(CURLcode *curl, char *url, struct MemoryStruct *chunk) {
  CURLcode result;

  chunk->memory = malloc(1);
  chunk->size = 0;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

  result = curl_easy_perform(curl);
  if (result != CURLE_OK)
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(result));

  return (int)result;
}

int smoggy_get_citydata(CURL *curl, struct SmoggyData *smoggydata,
                        const char *city_name) {
  char url[256]; // TODO: Do not use a hard-coded URL length!
  struct MemoryStruct chunk;

  char *city_name_url_encoded = curl_easy_escape(curl, city_name, 0);
  // clang-format off
  snprintf(url, sizeof(url), "https://geocoding-api.open-meteo.com/v1/search?name=%s&count=1&language=en&format=json", city_name_url_encoded);
  // clang-format on
  curl_free(city_name_url_encoded);
  CURLcode result = get_curl_data(curl, url, &chunk);
  if (result != CURLE_OK) {
    // fprintf(stderr, "get_curl_data() failed: %s\n",
    // curl_easy_strerror(result));
    free(chunk.memory);
    return EXIT_FAILURE;
  }

  cJSON *json = cJSON_Parse(chunk.memory);
  if (json == NULL) {
    const char *error_ptr = cJSON_GetErrorPtr();
    fprintf(stderr, "cJSON_Parse() failed: %s\n", error_ptr);
    free(chunk.memory);
    return EXIT_FAILURE;
  }

  cJSON *geocoding_results = cJSON_GetObjectItemCaseSensitive(json, "results");
  if (!cJSON_IsArray(geocoding_results)) {
    // TODO: Improve the message (e.g., "No city found")
    fprintf(stderr, "cJSON_GetObjectItemCaseSensitive() failed for results\n");
    cJSON_Delete(json);
    free(chunk.memory);
    return EXIT_FAILURE;
  }

  // clang-format off
  const char *geocoding_city_name = cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(geocoding_results, 0), "name")->valuestring;
  smoggydata->city_name = malloc(sizeof(char) * (strlen(geocoding_city_name) + 1));
  strcpy(smoggydata->city_name, geocoding_city_name);

  const char *geocoding_city_country_code = cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(geocoding_results, 0), "country_code")->valuestring;
  smoggydata->city_country_code = malloc(sizeof(char) * (strlen(geocoding_city_country_code) + 1));
  strcpy(smoggydata->city_country_code, geocoding_city_country_code);

  smoggydata->city_latitude  = cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(geocoding_results, 0), "latitude" )->valuedouble;
  smoggydata->city_longitude = cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(geocoding_results, 0), "longitude")->valuedouble;
  // clang-format on

  cJSON_Delete(json);
  free(chunk.memory);

  return EXIT_SUCCESS;
}

int smoggy_get_airqualitydata(CURL *curl, struct SmoggyData *smoggydata) {
  char url[256]; // TODO: Do not use a hard-coded URL length!
  struct MemoryStruct chunk;

  // clang-format off
  snprintf(url, sizeof(url), "https://air-quality-api.open-meteo.com/v1/air-quality?latitude=%f&longitude=%f&current=pm10,pm2_5,ozone,nitrogen_dioxide,sulphur_dioxide,carbon_monoxide,european_aqi,us_aqi&forecast_days=1", smoggydata->city_latitude, smoggydata->city_longitude);
  // clang-format on
  CURLcode result = get_curl_data(curl, url, &chunk);
  if (result != CURLE_OK) {
    // fprintf(stderr, "get_curl_data() failed: %s\n",
    // curl_easy_strerror(result));
    free(chunk.memory);
    return EXIT_FAILURE;
  }

  cJSON *json = cJSON_Parse(chunk.memory);
  if (json == NULL) {
    const char *error_ptr = cJSON_GetErrorPtr();
    fprintf(stderr, "cJSON_Parse() failed: %s\n", error_ptr);
    free(chunk.memory);
    return EXIT_FAILURE;
  }

  cJSON *airquality_current = cJSON_GetObjectItemCaseSensitive(json, "current");
  if (!cJSON_IsObject(airquality_current)) {
    fprintf(stderr, "cJSON_GetObjectItemCaseSensitive() failed for current\n");
    cJSON_Delete(json);
    free(chunk.memory);
    return EXIT_FAILURE;
  }

  cJSON *airquality_current_units =
      cJSON_GetObjectItemCaseSensitive(json, "current_units");
  if (!cJSON_IsObject(airquality_current_units)) {
    fprintf(stderr,
            "cJSON_GetObjectItemCaseSensitive() failed for current_units\n");
    cJSON_Delete(json);
    free(chunk.memory);
    return EXIT_FAILURE;
  }

  /* Values */
  // clang-format off
  smoggydata->european_aqi     = cJSON_GetObjectItemCaseSensitive(airquality_current, "european_aqi")     ->valueint;
  smoggydata->us_aqi           = cJSON_GetObjectItemCaseSensitive(airquality_current, "us_aqi")           ->valueint;
  smoggydata->pm10             = cJSON_GetObjectItemCaseSensitive(airquality_current, "pm10")             ->valuedouble;
  smoggydata->pm2_5            = cJSON_GetObjectItemCaseSensitive(airquality_current, "pm2_5")            ->valuedouble;
  smoggydata->ozone            = cJSON_GetObjectItemCaseSensitive(airquality_current, "ozone")            ->valuedouble;
  smoggydata->nitrogen_dioxide = cJSON_GetObjectItemCaseSensitive(airquality_current, "nitrogen_dioxide") ->valuedouble;
  smoggydata->sulphur_dioxide  = cJSON_GetObjectItemCaseSensitive(airquality_current, "sulphur_dioxide")  ->valuedouble;
  smoggydata->carbon_monoxide  = cJSON_GetObjectItemCaseSensitive(airquality_current, "carbon_monoxide")  ->valuedouble;
  // clang-format on

  /* Units */
  // clang-format off
  const char *smoggydata_european_aqi_unit     = cJSON_GetObjectItemCaseSensitive(airquality_current_units, "european_aqi")    ->valuestring;
  const char *smoggydata_us_aqi_unit           = cJSON_GetObjectItemCaseSensitive(airquality_current_units, "us_aqi")          ->valuestring;
  const char *smoggydata_pm10_unit             = cJSON_GetObjectItemCaseSensitive(airquality_current_units, "pm10")            ->valuestring;
  const char *smoggydata_pm2_5_unit            = cJSON_GetObjectItemCaseSensitive(airquality_current_units, "pm2_5")           ->valuestring;
  const char *smoggydata_ozone_unit            = cJSON_GetObjectItemCaseSensitive(airquality_current_units, "ozone")           ->valuestring;
  const char *smoggydata_nitrogen_dioxide_unit = cJSON_GetObjectItemCaseSensitive(airquality_current_units, "nitrogen_dioxide")->valuestring;
  const char *smoggydata_sulphur_dioxide_unit  = cJSON_GetObjectItemCaseSensitive(airquality_current_units, "sulphur_dioxide") ->valuestring;
  const char *smoggydata_carbon_monoxide_unit  = cJSON_GetObjectItemCaseSensitive(airquality_current_units, "carbon_monoxide") ->valuestring;
  smoggydata->european_aqi_unit     = malloc(sizeof(char) * (strlen(smoggydata_european_aqi_unit    ) + 1));
  smoggydata->us_aqi_unit           = malloc(sizeof(char) * (strlen(smoggydata_us_aqi_unit          ) + 1));
  smoggydata->pm10_unit             = malloc(sizeof(char) * (strlen(smoggydata_pm10_unit            ) + 1));
  smoggydata->pm2_5_unit            = malloc(sizeof(char) * (strlen(smoggydata_pm2_5_unit           ) + 1));
  smoggydata->ozone_unit            = malloc(sizeof(char) * (strlen(smoggydata_ozone_unit           ) + 1));
  smoggydata->nitrogen_dioxide_unit = malloc(sizeof(char) * (strlen(smoggydata_nitrogen_dioxide_unit) + 1));
  smoggydata->sulphur_dioxide_unit  = malloc(sizeof(char) * (strlen(smoggydata_sulphur_dioxide_unit ) + 1));
  smoggydata->carbon_monoxide_unit  = malloc(sizeof(char) * (strlen(smoggydata_carbon_monoxide_unit ) + 1));
  strcpy(smoggydata->european_aqi_unit    , smoggydata_european_aqi_unit    );
  strcpy(smoggydata->us_aqi_unit          , smoggydata_us_aqi_unit          );
  strcpy(smoggydata->pm10_unit            , smoggydata_pm10_unit            );
  strcpy(smoggydata->pm2_5_unit           , smoggydata_pm2_5_unit           );
  strcpy(smoggydata->ozone_unit           , smoggydata_ozone_unit           );
  strcpy(smoggydata->nitrogen_dioxide_unit, smoggydata_nitrogen_dioxide_unit);
  strcpy(smoggydata->sulphur_dioxide_unit , smoggydata_sulphur_dioxide_unit );
  strcpy(smoggydata->carbon_monoxide_unit , smoggydata_carbon_monoxide_unit );
  // clang-format on

  cJSON_Delete(json);
  free(chunk.memory);

  return EXIT_SUCCESS;
}

void smoggy_print(struct SmoggyData *smoggydata) {
  // printf("Name: %s\n", smoggydata->city_name);
  // printf("Code: %s\n", smoggydata->city_country_code);
  // printf("Latitude: %f\n", smoggydata->city_latitude);
  // printf("Longitude: %f\n", smoggydata->city_longitude);

  // clang-format off
  printf("Location%s%s (%s)\n",             separator, smoggydata->city_name       , smoggydata->city_country_code    );
  // printf("European AQI%s%d %s\n",           separator, smoggydata->european_aqi    , smoggydata->european_aqi_unit    );
  // printf("U.S. AQI%s%d %s\n",               separator, smoggydata->us_aqi          , smoggydata->us_aqi_unit          );
  printf("PM₁₀%s%.1f %s\n",                 separator, smoggydata->pm10            , smoggydata->pm10_unit            );
  printf("PM₂.₅%s%.1f %s\n",                separator, smoggydata->pm2_5           , smoggydata->pm2_5_unit           );
  printf("Ozone 0₃%s%.1f %s\n",             separator, smoggydata->ozone           , smoggydata->ozone_unit           );
  printf("Nitrogen Dioxide NO₂%s%.1f %s\n", separator, smoggydata->nitrogen_dioxide, smoggydata->nitrogen_dioxide_unit);
  printf("Sulphur Dioxide SO₂%s%.1f %s\n",  separator, smoggydata->sulphur_dioxide , smoggydata->sulphur_dioxide_unit );
  printf("Carbon Monoxide CO%s%.1f %s\n",   separator, smoggydata->carbon_monoxide , smoggydata->carbon_monoxide_unit );
  // clang-format on
}

int main(int argc, char *argv[]) {
  CURL *curl;
  CURLcode result;
  int smoggy_code = EXIT_SUCCESS;

  // TODO: Improve cli argument handling.
  if (argc > 2) {
    fprintf(stderr, "%s accepts exactly one argument.\n", argv[0]);
    return EXIT_FAILURE;
  }

  result = curl_global_init(CURL_GLOBAL_ALL);
  if (result != CURLE_OK) {
    fprintf(stderr, "curl_global_init() failed: %s\n",
            curl_easy_strerror(result));
    return EXIT_FAILURE;
  }

  curl = curl_easy_init();
  if (curl == NULL) {
    fprintf(stderr, "curl_easy_init() failed\n");
    curl_global_cleanup();
    return EXIT_FAILURE;
  }

  struct SmoggyData *smoggydata = smoggy_init();
  // TODO: Maybe it would be good to let users choose a city from the list of
  // results (instead of using the first one on the list)?
  smoggy_code =
      smoggy_get_citydata(curl, smoggydata, argv[1] ? argv[1] : "Warsaw");
  if (smoggy_code == EXIT_FAILURE)
    goto cleanup;

  smoggy_code = smoggy_get_airqualitydata(curl, smoggydata);
  if (smoggy_code == EXIT_FAILURE)
    goto cleanup;

  smoggy_print(smoggydata);

cleanup:
  smoggy_cleanup(smoggydata);
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  return smoggy_code;
}
