#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"

void addOrUpdateAttribute(cJSON *object, const char *key, cJSON *value);
void mergeObjects(cJSON *targetObject, cJSON *sourceObject);
void addOrUpdate(cJSON *dataObject, const char *key, cJSON *newObject);
cJSON *extractObjectFromString(const char *jsonString, const char *key);
cJSON *readJsonFromFile(const char *filename);
int writeJsonToFile(const char *filename, const char *jsonString);
void save_cjson_object_to_file(const char *filename, cJSON *jsonObject);
int updateJsonData(char *filename, char *macAddress, char *newObjectString);