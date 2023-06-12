#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"
#include "cJSON.c"

void addOrUpdateAttribute(cJSON *object, const char *key, cJSON *value);
void mergeObjects(cJSON *targetObject, cJSON *sourceObject);
void addOrUpdate(cJSON *dataObject, const char *key, cJSON *newObject);
cJSON *extractObjectFromString(const char *jsonString, const char *key);
int writeJsonToFile(const char *filename, const char *jsonString);
char *readJsonFromFile(const char *filename);
int updateJsonData(const char *filename, const char *desiredKey, const char *newObjectString);