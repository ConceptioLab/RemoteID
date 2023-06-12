#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"
#include "cJSON.c"

void addOrUpdateAttribute(cJSON *object, const char *key, cJSON *value);
void mergeObjects(cJSON *targetObject, cJSON *sourceObject);
void addOrUpdate(cJSON *dataObject, const char *key, cJSON *newObject);
cJSON *extractObjectFromString(const char *jsonString, const char *key);
cJSON *parseJsonFromFile(const char *filename);
void parse_json(char *newJson, char *macAddress);