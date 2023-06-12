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

char *readJsonFromFile(const char *filename)
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

    return buffer;
}

int writeJsonToFile(const char *filename, const char *jsonString)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        printf("Erro ao abrir o arquivo '%s' para escrita.\n", filename);
        return 0;
    }

    size_t bytesWritten = fwrite(jsonString, sizeof(char), strlen(jsonString), file);
    fclose(file);

    if (bytesWritten != strlen(jsonString))
    {
        printf("Erro ao escrever no arquivo '%s'.\n", filename);
        return 0;
    }

    return 1;
}

int updateJsonData(const char *filename, const char *desiredKey, const char *newObjectString)
{
    // Ler o JSON do arquivo
    char *jsonString = readJsonFromFile(filename);
    if (jsonString == NULL)
    {
        // Se não for possível ler o arquivo, criar um novo objeto cJSON vazio
        cJSON *dataObject = cJSON_CreateObject();
        if (dataObject == NULL)
        {
            printf("Erro ao criar objeto cJSON principal.\n");
            return 1;
        }

        // Converter o objeto cJSON em string formatada
        char *formattedJson = NULL;

        // Verificar se a chave desejada já existe no objeto principal
        cJSON *desiredObject = extractObjectFromString(jsonString, desiredKey);
        if (desiredObject != NULL)
        {
            // Se a chave já existir, mesclar os dados do novo objeto
            cJSON *newObject = cJSON_Parse(newObjectString);
            if (newObject == NULL)
            {
                printf("Erro ao fazer o parse do novo objeto JSON.\n");
                free(jsonString);
                cJSON_Delete(dataObject);
                return 1;
            }

            addOrUpdate(desiredObject, desiredKey, newObject);
            cJSON_Delete(newObject);

            // Atualizar o objeto principal com o objeto modificado
            addOrUpdate(dataObject, desiredKey, desiredObject);
            cJSON_Delete(desiredObject);

            // Converter o objeto cJSON em string formatada
            formattedJson = cJSON_Print(dataObject);
        }
        else
        {
            // Se a chave não existir, adicionar a nova chave com os valores fornecidos
            cJSON *newObject = cJSON_Parse(newObjectString);
            if (newObject == NULL)
            {
                printf("Erro ao fazer o parse do novo objeto JSON.\n");
                free(jsonString);
                cJSON_Delete(dataObject);
                return 1;
            }

            addOrUpdate(dataObject, desiredKey, newObject);
            cJSON_Delete(newObject);

            // Converter o objeto cJSON em string formatada
            formattedJson = cJSON_Print(dataObject);
        }

        // Salvar o JSON atualizado no arquivo
        int success = writeJsonToFile(filename, formattedJson);
        if (success)
        {
            printf("JSON atualizado e salvo no arquivo '%s'.\n", filename);
        }
        else
        {
            printf("Erro ao salvar o JSON atualizado no arquivo '%s'.\n", filename);
        }

        // Liberar a memória
        cJSON_Delete(dataObject);
        free(jsonString);
        free(formattedJson);
    }
    else
    {
        printf("Erro ao ler o arquivo JSON '%s'.\n", filename);
        return 1;
    }

    return 0;
}