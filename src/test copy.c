#include <stdio.h>
#include <stdlib.h>
#include "./include/cJSON/cJSON.h"
#include "./include/cJSON/cJSON.c"

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

// Primeiro:     Ler e separar MAC dos atributos.
// Segundo:      Criar objeto com o MAC e os atributos novos.
// Terceiro:     Adicionar ou atualizar o MAC com a string recebida.
// Quarto:       Salvar no arquivo.

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

// Receber string json a ser adicionada em mac
// Transformar em objeto cJSON
// addOrUpdate do objeto principal
// verifica o array
// Salva

int main()
{

        // Criar objeto cJSON principal
        cJSON *dataObject = cJSON_CreateObject();
        if (dataObject == NULL)
        {
                printf("Erro ao criar objeto cJSON principal.\n");
                return 1;
        }

        // Exemplo de objetos JSON em formato de string
        const char *objectString1 = "{\"atributo1\":\"valor1\",\"atributo2\":123}";
        const char *objectString2 = "{\"atributo2\":456,\"atributo3\":\"valor3\"}";
        const char *objectString3 = "{\"atributo2\":\"novo_valor\",\"atributo1\":true}";

        // Converter as strings em objetos cJSON
        cJSON *newObject1 = cJSON_Parse(objectString1);
        cJSON *newObject2 = cJSON_Parse(objectString2);
        cJSON *newObject3 = cJSON_Parse(objectString3);

        // Adicionar os objetos ao objeto principal
        addOrUpdate(dataObject, "key1", newObject1);
        addOrUpdate(dataObject, "key2", newObject2);
        addOrUpdate(dataObject, "key1", newObject3);

        // Verificar os atributos dinamicamente e substituir os valores correspondentes
        cJSON *object = NULL;
        cJSON_ArrayForEach(object, dataObject)
        {
                cJSON *attribute = NULL;
                cJSON_ArrayForEach(attribute, object)
                {
                        cJSON *existingAttribute = cJSON_GetObjectItemCaseSensitive(dataObject, attribute->string);
                        if (existingAttribute != NULL && cJSON_IsString(existingAttribute) && cJSON_IsString(attribute))
                        {
                                cJSON_ReplaceItemInObjectCaseSensitive(dataObject, attribute->string, cJSON_Duplicate(attribute, 1));
                        }
                }
        }

        // Imprimir o JSON resultante
        char *formattedJson = cJSON_Print(dataObject);
        printf("%s\n", formattedJson);
        free(formattedJson);

        // Liberar a memória
        cJSON_Delete(dataObject);

        const char *jsonString = "{"
                                 "    \"key1\": {"
                                 "        \"atributo1\": \"valor1\","
                                 "        \"atributo2\": 456,"
                                 "        \"atributo3\": \"valor3\""
                                 "    },"
                                 "    \"key2\": {"
                                 "        \"atributo1\": \"novo_valor\","
                                 "        \"atributo4\": true"
                                 "    }"
                                 "}";

        const char *desiredKey = "key2";

        cJSON *desiredObject = extractObjectFromString(jsonString, desiredKey);
        if (desiredObject != NULL)
        {
                char *formattedJson = cJSON_Print(desiredObject);
                printf("Objeto separado:\n%s\n", formattedJson);
                free(formattedJson);
        }

        return 0;
}