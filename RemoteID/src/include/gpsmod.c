#include "gpsmod.h"
#include "opendroneid.h"
#include "../bluetooth/remote.h"
#include <math.h>
#include "./cJSON/cJSON.h"

int init_gps(struct fixsource_t *source, struct gps_data_t *gpsdata)
{
    unsigned int flags = WATCH_ENABLE;

    gpsd_source_spec(NULL, source);

    if (gps_open(source->server, source->port, gpsdata) < 0)
    {
        fprintf(stderr, "Falha ao abrir a conexão com o GPS.\n");
        return 1;
    }
    else
        printf("Porta aberta\n");

    if (NULL != source->device)
    {
        flags |= WATCH_DEVICE;
    }

    gps_stream(gpsdata, flags, NULL);

    return 0;
}

void process_gps_data(struct gps_data_t *gpsdata, struct ODID_UAS_Data *uasData)
{
    // Ler o arquivo JSON existente
    FILE *json_file = fopen("../../uav.json", "r");
    if (!json_file)
    {
        printf("Erro ao abrir o arquivo JSON para leitura.\n");
    }

    // Obter o tamanho do arquivo
    fseek(json_file, 0, SEEK_END);
    long file_size = ftell(json_file);
    fseek(json_file, 0, SEEK_SET);

    // Ler o conteúdo do arquivo para uma string
    char *json_content = (char *)malloc(file_size + 1);
    fread(json_content, 1, file_size, json_file);
    fclose(json_file);
    json_content[file_size] = '\0';

    // Analisar o conteúdo JSON
    cJSON *root = cJSON_Parse(json_content);
    free(json_content);

    if (!root)
    {
        printf("Erro ao analisar o conteúdo JSON.\n");
    }

    // Obter ou adicionar um objeto "coordinates"
    cJSON *coordinates = cJSON_GetObjectItemCaseSensitive(root, "coordinates");
    if (!coordinates)
    {
        coordinates = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "coordinates", coordinates);
    }
    if (gpsdata->fix.mode >= MODE_NO_FIX)
    {
        if ((gpsdata->fix.latitude > 0.00001 && gpsdata->fix.longitude > 0.00001) ||
            (gpsdata->fix.latitude < -0.00001 && gpsdata->fix.longitude < 0.00001))
        {

            uasData->Location.Latitude = gpsdata->fix.latitude;
            uasData->Location.Longitude = gpsdata->fix.longitude;

            // Modificar os valores das coordenadas GPS
            cJSON *lat_node = cJSON_GetObjectItemCaseSensitive(coordinates, "latitude");
            cJSON *lon_node = cJSON_GetObjectItemCaseSensitive(coordinates, "longitude");

            if (!lat_node)
            {
                lat_node = cJSON_CreateNumber(gpsdata->fix.latitude);
                cJSON_AddItemToObject(coordinates, "latitude", lat_node);
            }
            else
            {
                cJSON_SetNumberValue(lat_node, gpsdata->fix.latitude);
            }

            if (!lon_node)
            {
                lon_node = cJSON_CreateNumber(gpsdata->fix.longitude);
                cJSON_AddItemToObject(coordinates, "longitude", lon_node);
            }
            else
            {
                cJSON_SetNumberValue(lon_node, gpsdata->fix.longitude);
            }

            if (first == 0 || first < 2)
            {
                first++;
            }

            if (first == 1)
            {
                printf("Set operator location\n");
                uasData->System.OperatorLatitude = uasData->Location.Latitude;
                uasData->System.OperatorLongitude = uasData->Location.Longitude;
                first++;
            }
        }
    }

    if (gpsdata->fix.mode >= MODE_3D)
    {
        if (isfinite(gpsdata->fix.speed))
        {
            uasData->Location.SpeedHorizontal = gpsdata->fix.speed;
            // Modificar os valores das coordenadas GPS
            cJSON *speed_h_node = cJSON_GetObjectItemCaseSensitive(coordinates, "speed_horizontal");

            if (!speed_h_node)
            {
                speed_h_node = cJSON_CreateNumber(gpsdata->fix.speed);
                cJSON_AddItemToObject(coordinates, "speed_horizontal", speed_h_node);
            }
            else
            {
                cJSON_SetNumberValue(speed_h_node, gpsdata->fix.speed);
            }
        }

        if (isfinite(gpsdata->fix.climb))
        {
            uasData->Location.SpeedVertical = gpsdata->fix.climb;
            // Modificar os valores das coordenadas GPS
            cJSON *speed_v_node = cJSON_GetObjectItemCaseSensitive(coordinates, "speed_vertical");

            if (!speed_v_node)
            {
                speed_v_node = cJSON_CreateNumber(gpsdata->fix.climb);
                cJSON_AddItemToObject(coordinates, "speed_vertical", speed_v_node);
            }
            else
            {
                cJSON_SetNumberValue(speed_v_node, gpsdata->fix.climb);
            }
        }

        uasData->Location.HeightType = ODID_HEIGHT_REF_OVER_GROUND;
        cJSON *height_type_node = cJSON_GetObjectItemCaseSensitive(coordinates, "height_type");
        if (!height_type_node)
        {
            height_type_node = cJSON_CreateNumber(ODID_HEIGHT_REF_OVER_GROUND);
            cJSON_AddItemToObject(coordinates, "height_type", height_type_node);
        }
        else
        {
            cJSON_SetNumberValue(height_type_node, ODID_HEIGHT_REF_OVER_GROUND);
        }

        if (isfinite(gpsdata->fix.track))
        {
            uasData->Location.Direction = gpsdata->fix.track;
            cJSON *direction_node = cJSON_GetObjectItemCaseSensitive(coordinates, "direction");
            if (!direction_node)
            {
                direction_node = cJSON_CreateNumber(gpsdata->fix.track);
                cJSON_AddItemToObject(coordinates, "direction", direction_node);
            }
            else
            {
                cJSON_SetNumberValue(direction_node, gpsdata->fix.track);
            }
        }
        if (isfinite(gpsdata->fix.altMSL))
        {
            uasData->Location.AltitudeGeo = gpsdata->fix.altMSL;
            uasData->Location.AltitudeBaro = gpsdata->fix.altMSL;
            uasData->Location.Height =
                gpsdata->fix.altMSL - uasData->System.OperatorAltitudeGeo;
        }
        else if (isfinite(gpsdata->fix.altitude))
        {
            uasData->Location.AltitudeGeo = gpsdata->fix.altitude;
            uasData->Location.AltitudeBaro = gpsdata->fix.altitude;
            uasData->Location.Height =
                gpsdata->fix.altitude - uasData->System.OperatorAltitudeGeo;
        }
        if (isfinite(gpsdata->fix.altitude) || isfinite(gpsdata->fix.altMSL))
        {
            cJSON *altitudegeo_node = cJSON_GetObjectItemCaseSensitive(coordinates, "altitudegeo");
            cJSON *altitudebaro_node = cJSON_GetObjectItemCaseSensitive(coordinates, "altitudebaro");
            cJSON *height_node = cJSON_GetObjectItemCaseSensitive(coordinates, "height");
            if (!altitudegeo_node)
            {
                altitudegeo_node = cJSON_CreateNumber(uasData->Location.AltitudeGeo);
                cJSON_AddItemToObject(coordinates, "altitudegeo", altitudegeo_node);
            }
            else
            {
                cJSON_SetNumberValue(altitudegeo_node, uasData->Location.AltitudeGeo);
            }
            if (!altitudebaro_node)
            {
                altitudebaro_node = cJSON_CreateNumber(uasData->Location.AltitudeBaro);
                cJSON_AddItemToObject(coordinates, "altitudebaro", altitudebaro_node);
            }
            else
            {
                cJSON_SetNumberValue(altitudebaro_node, uasData->Location.AltitudeBaro);
            }
            if (!height_node)
            {
                height_node = cJSON_CreateNumber(uasData->Location.Height);
                cJSON_AddItemToObject(coordinates, "height", height_node);
            }
            else
            {
                cJSON_SetNumberValue(height_node, uasData->Location.Height);
            }
        }

        if (isfinite(gpsdata->fix.eph))
        {
            uasData->Location.HorizAccuracy = createEnumHorizontalAccuracy(gpsdata->fix.eph);
            cJSON *horizAccuracy_node = cJSON_GetObjectItemCaseSensitive(coordinates, "horizAccuracy");
            if (!horizAccuracy_node)
            {
                horizAccuracy_node = cJSON_CreateNumber(uasData->Location.HorizAccuracy);
                cJSON_AddItemToObject(coordinates, "horizAccuracy", horizAccuracy_node);
            }
            else
            {
                cJSON_SetNumberValue(horizAccuracy_node, uasData->Location.HorizAccuracy);
            }
        }

        if (isfinite(gpsdata->fix.epy))
        {
            uasData->Location.VertAccuracy = createEnumVerticalAccuracy(gpsdata->fix.epy);
            cJSON *vertAccuracy_node = cJSON_GetObjectItemCaseSensitive(coordinates, "vertAccuracy");
            if (!vertAccuracy_node)
            {
                vertAccuracy_node = cJSON_CreateNumber(uasData->Location.VertAccuracy);
                cJSON_AddItemToObject(coordinates, "vertAccuracy", vertAccuracy_node);
            }
            else
            {
                cJSON_SetNumberValue(vertAccuracy_node, uasData->Location.VertAccuracy);
            }
        }

        if (isfinite(gpsdata->fix.eps))
        {
            uasData->Location.SpeedAccuracy = createEnumSpeedAccuracy(gpsdata->fix.eps);
            cJSON *speedAccuracy_node = cJSON_GetObjectItemCaseSensitive(coordinates, "speedAccuracy");
            if (!speedAccuracy_node)
            {
                speedAccuracy_node = cJSON_CreateNumber(uasData->Location.SpeedAccuracy);
                cJSON_AddItemToObject(coordinates, "speedAccuracy", speedAccuracy_node);
            }
            else
            {
                cJSON_SetNumberValue(speedAccuracy_node, uasData->Location.SpeedAccuracy);
            }
        }
    }

    // Salvar o objeto JSON atualizado de volta no arquivo
    json_file = fopen("../../uav.json", "w");
    if (json_file)
    {
        char *json_str = cJSON_Print(root);
        fprintf(json_file, "%s\n", json_str);
        fclose(json_file);
        cJSON_free(json_str);
    }
    else
    {
        printf("Erro ao abrir o arquivo JSON para escrita.\n");
    }
    // Libere os recursos alocados pela biblioteca cJSON
    cJSON_Delete(root);
}