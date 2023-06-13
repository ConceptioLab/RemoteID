#include "json.h"

void addOrUpdateAttribute(cJSON *object, const char *key, cJSON *value)
{
    cJSON *existingAttribute = cJSON_GetObjectItemCaseSensitive(object, key);
    if (existingAttribute != NULL)
    {
        cJSON_ReplaceItemInObjectCaseSensitive(object, key, cJSON_Duplicate(value, 1));
    }
    else
    {
        cJSON_AddItemToObject(object, key, cJSON_Duplicate(value, 1));
    }
}

void mergeObjects(cJSON *targetObject, cJSON *sourceObject)
{
    cJSON *child = NULL;
    cJSON_ArrayForEach(child, sourceObject)
    {
        addOrUpdateAttribute(targetObject, child->string, child);
    }
}

void addOrUpdate(cJSON *dataObject, const char *key, cJSON *newObject)
{
    cJSON *existingObject = cJSON_GetObjectItemCaseSensitive(dataObject, key);
    if (existingObject != NULL && cJSON_IsObject(existingObject) && cJSON_IsObject(newObject))
    {
        mergeObjects(existingObject, newObject);
        cJSON_Delete(newObject);
    }
    else
    {
        cJSON_AddItemToObject(dataObject, key, newObject);
    }
}

cJSON *extractObjectFromString(const char *jsonString, const char *key)
{
    cJSON *root = cJSON_Parse(jsonString);
    if (root == NULL)
    {
        printf("Erro ao fazer o parse da string JSON.\n");
        return NULL;
    }

    cJSON *object = cJSON_GetObjectItemCaseSensitive(root, key);
    if (object == NULL)
    {
        printf("Objeto com a chave '%s' não encontrado.\n", key);
        cJSON_Delete(root);
        return NULL;
    }

    // Remover o objeto do root para que possa ser usado separadamente
    cJSON_DetachItemViaPointer(root, object);

    // Liberar a memória do root, mas manter o objeto separado
    cJSON_Delete(root);

    return object;
}

cJSON *readJsonFromFile(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Erro ao abrir o arquivo '%s'.\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc((fileSize + 1) * sizeof(char));
    if (buffer == NULL)
    {
        printf("Erro ao alocar memória para o buffer.\n");
        fclose(file);
        return NULL;
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize)
    {
        printf("Erro ao ler o arquivo '%s'.\n", filename);
        fclose(file);
        free(buffer);
        return NULL;
    }

    buffer[fileSize] = '\0';

    fclose(file);

    cJSON *jsonObject = cJSON_Parse(buffer);
    free(buffer);

    if (jsonObject == NULL)
    {
        printf("Erro ao fazer o parse do arquivo JSON.\n");
        return NULL;
    }

    return jsonObject;
}

void save_cjson_object_to_file(const char *filename, cJSON *jsonObject)
{
    if (jsonObject == NULL)
    {
        printf("Objeto cJSON inválido.\n");
        return;
    }

    char *formattedJson = cJSON_Print(jsonObject);
    if (formattedJson == NULL)
    {
        printf("Erro ao formatar o objeto cJSON.\n");
        return;
    }
    printf("%s\n", formattedJson);

    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        printf("Erro ao abrir o arquivo para escrita: %s\n", filename);
        free(formattedJson);
        return;
    }

    fputs(formattedJson, file);
    fclose(file);
    free(formattedJson);
}

int updateJsonData(char *filename, char *macAddress, char *newObjectString)
{
    // Ler o JSON do arquivo
    cJSON *mainObject = readJsonFromFile(filename);

    // Criar objeto cJSON principal
    if (mainObject == NULL)
    {
        printf("JSON não existe, criando JSON.\n");
        mainObject = cJSON_CreateObject();
    }

    // Criar novo objeto com a string.
    cJSON *newObject = cJSON_Parse(newObjectString);
    if (newObject == NULL)
    {
        printf("Erro ao fazer o parse da string JSON.\n");
        return 1;
    }

    // Adicionar os objetos ao objeto principal
    addOrUpdate(mainObject, macAddress, newObject);

    // Verificar os atributos dinamicamente e substituir os valores correspondentes
    cJSON *object = NULL;
    cJSON_ArrayForEach(object, mainObject)
    {
        cJSON *attribute = NULL;
        cJSON_ArrayForEach(attribute, object)
        {
            cJSON *existingAttribute = cJSON_GetObjectItemCaseSensitive(mainObject, attribute->string);
            if (existingAttribute != NULL && cJSON_IsString(existingAttribute) && cJSON_IsString(attribute))
            {
                cJSON_ReplaceItemInObjectCaseSensitive(mainObject, attribute->string, cJSON_Duplicate(attribute, 1));
            }
        }
    }

    // Imprimir o JSON resultante
    save_cjson_object_to_file(filename, mainObject);

    // Liberar a memória
    cJSON_Delete(mainObject);
    return 0;
}