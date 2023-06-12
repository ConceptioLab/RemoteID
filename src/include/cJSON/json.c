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

// Primeiro:     Ler e separar MAC dos atributos.
// Segundo:      Criar objeto com o MAC e os atributos novos.
// Terceiro:     Adicionar ou atualizar o MAC com a string recebida.
// Quarto:       Salvar no arquivo.

cJSON *parseJsonFromFile(const char *filename)
{
        FILE *file = fopen(filename, "r");
        if (file == NULL)
        {
                printf("Erro ao abrir o arquivo '%s' para leitura.\n", filename);
                return NULL;
        }

        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        rewind(file);

        char *fileContent = (char *)malloc(fileSize + 1);
        if (fileContent == NULL)
        {
                printf("Erro ao alocar memória para o conteúdo do arquivo.\n");
                fclose(file);
                return NULL;
        }

        fread(fileContent, 1, fileSize, file);
        fileContent[fileSize] = '\0';
        fclose(file);

        cJSON *jsonObject = cJSON_Parse(fileContent);
        free(fileContent);

        if (jsonObject == NULL)
        {
                printf("Erro ao fazer o parse do arquivo JSON.\n");
                return NULL;
        }

        return jsonObject;
}

void parse_json(char *newJson, char *macAddress)
{
        const char *filename = "dados.json";
        char *formattedJson;

        cJSON *jsonObject = parseJsonFromFile(filename);
        if (jsonObject != NULL)
        {
                const char *jsonMainString = cJSON_Print(jsonObject);

                cJSON *desiredObject = extractObjectFromString(jsonMainString, macAddress);
                if (desiredObject != NULL)
                {
                        formattedJson = cJSON_Print(desiredObject);
                }

                // Converter as strings em objetos cJSON
                cJSON *mainObject = cJSON_Parse(jsonMainString);
                cJSON *newObject = cJSON_Parse(newJson);

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
                FILE *file = fopen(filename, "w");
                formattedJson = cJSON_Print(mainObject);

                fputs(formattedJson, file);

                printf("%s\n", formattedJson);
                // Liberar a memória
                cJSON_Delete(mainObject);
                cJSON_Delete(newObject);
                free(formattedJson);
                fclose(file);
        }
        else
        {
                // Criar objeto cJSON principal
                cJSON *dataObject = cJSON_CreateObject();
                if (dataObject == NULL)
                {
                        printf("Erro ao criar objeto cJSON principal.\n");
                }
                cJSON *newObject = cJSON_Parse(newJson);
                addOrUpdate(dataObject, macAddress, newObject);
                FILE *file = fopen(filename, "w");
                if (file != NULL)
                {
                        formattedJson = cJSON_Print(dataObject);
                        fputs(formattedJson, file);
                        fclose(file);
                        printf("%s\n", formattedJson);
                }
                else
                {
                        printf("Erro ao abrir o arquivo para escrita.\n");
                }
                cJSON_Delete(newObject);
                free(formattedJson);
                fclose(file);
        }
}