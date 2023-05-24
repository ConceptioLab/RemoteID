#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/param.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "opendroneid.h"
#include "opendroneid.c"
#include "print_bt_features.h"

#define BASIC_ID_POS_ZERO 0
#define BASIC_ID_POS_ONE 1

#define ODID_MESSAGE_SIZE 25

int device_descriptor = 0;

static void fill_example_data(struct ODID_UAS_Data *uasData) {
    uasData->BasicID[BASIC_ID_POS_ZERO].UAType = ODID_UATYPE_HELICOPTER_OR_MULTIROTOR;
    uasData->BasicID[BASIC_ID_POS_ZERO].IDType = ODID_IDTYPE_SERIAL_NUMBER;
    char uas_id[] = "555555555555555555AB";
    strncpy(uasData->BasicID[BASIC_ID_POS_ZERO].UASID, uas_id,
            MINIMUM(sizeof(uas_id), sizeof(uasData->BasicID[BASIC_ID_POS_ZERO].UASID)));

    uasData->BasicID[BASIC_ID_POS_ONE].UAType = ODID_UATYPE_HELICOPTER_OR_MULTIROTOR;
    uasData->BasicID[BASIC_ID_POS_ONE].IDType = ODID_IDTYPE_SPECIFIC_SESSION_ID;
    char uas_caa_id[] = "7272727727272772720A";
    strncpy(uasData->BasicID[BASIC_ID_POS_ONE].UASID, uas_caa_id,
            MINIMUM(sizeof(uas_caa_id), sizeof(uasData->BasicID[BASIC_ID_POS_ONE].UASID)));

    uasData->Auth[0].AuthType = ODID_AUTH_UAS_ID_SIGNATURE;
    uasData->Auth[0].DataPage = 0;
    uasData->Auth[0].LastPageIndex = 2;
    uasData->Auth[0].Length = 63;
    uasData->Auth[0].Timestamp = 28000000;
    char auth0_data[] = "15648975321685249";
    memcpy(uasData->Auth[0].AuthData, auth0_data,
           MINIMUM(sizeof(auth0_data), sizeof(uasData->Auth[0].AuthData)));

    uasData->Auth[1].AuthType = ODID_AUTH_UAS_ID_SIGNATURE;
    uasData->Auth[1].DataPage = 1;
    char auth1_data[] = "36289078124690153452713";
    memcpy(uasData->Auth[1].AuthData, auth1_data,
           MINIMUM(sizeof(auth1_data), sizeof(uasData->Auth[1].AuthData)));

    uasData->Auth[2].AuthType = ODID_AUTH_UAS_ID_SIGNATURE;
    uasData->Auth[2].DataPage = 2;
    char auth2_data[] = "12345678901234567890123";
    memcpy(uasData->Auth[2].AuthData, auth2_data,
           MINIMUM(sizeof(auth2_data), sizeof(uasData->Auth[2].AuthData)));

    uasData->SelfID.DescType = ODID_DESC_TYPE_TEXT;
    char description[] = "Drone ID test flight---";
    strncpy(uasData->SelfID.Desc, description,
            MINIMUM(sizeof(description), sizeof(uasData->SelfID.Desc)));

    uasData->System.OperatorLocationType = ODID_OPERATOR_LOCATION_TYPE_TAKEOFF;
    uasData->System.ClassificationType = ODID_CLASSIFICATION_TYPE_EU;
    uasData->System.OperatorLatitude = uasData->Location.Latitude + 0.001;
    uasData->System.OperatorLongitude = uasData->Location.Longitude - 0.001;
    uasData->System.AreaCount = 1;
    uasData->System.AreaRadius = 0;
    uasData->System.AreaCeiling = 0;
    uasData->System.AreaFloor = 0;
    uasData->System.CategoryEU = ODID_CATEGORY_EU_OPEN;
    uasData->System.ClassEU = ODID_CLASS_EU_CLASS_1;
    uasData->System.OperatorAltitudeGeo = 20.5f;
    uasData->System.Timestamp = 28056789;

    uasData->OperatorID.OperatorIdType = ODID_OPERATOR_ID;
    char operatorId[] = "FIN87astrdge12k8";
    strncpy(uasData->OperatorID.OperatorId, operatorId,
            MINIMUM(sizeof(operatorId), sizeof(uasData->OperatorID.OperatorId)));
}

static void fill_example_gps_data(struct ODID_UAS_Data *uasData) {
    uasData->Location.Status = ODID_STATUS_AIRBORNE;
    uasData->Location.Direction = 361.f;
    uasData->Location.SpeedHorizontal = 0.0f;
    uasData->Location.SpeedVertical = 0.35f;
    uasData->Location.Latitude = 51.4791;
    uasData->Location.Longitude = -0.0013;
    uasData->Location.AltitudeBaro = 100;
    uasData->Location.AltitudeGeo = 110;
    uasData->Location.HeightType = ODID_HEIGHT_REF_OVER_GROUND;
    uasData->Location.Height = 80;
    uasData->Location.HorizAccuracy = createEnumHorizontalAccuracy(5.5f);
    uasData->Location.VertAccuracy = createEnumVerticalAccuracy(9.5f);
    uasData->Location.BaroAccuracy = createEnumVerticalAccuracy(0.5f);
    uasData->Location.SpeedAccuracy = createEnumSpeedAccuracy(0.5f);
    uasData->Location.TSAccuracy = createEnumTimestampAccuracy(0.1f);
    uasData->Location.TimeStamp = 360.52f;
}


static int open_hci_device()
{
    struct hci_filter flt; // Host Controller Interface filter

    int dev_id = hci_devid("hci0");
    if (dev_id < 0)
        dev_id = hci_get_route(NULL);

    int dd = hci_open_dev(dev_id);
    if (dd < 0)
    {
        perror("Device open failed");
        exit(EXIT_FAILURE);
    }

    hci_filter_clear(&flt);
    hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
    hci_filter_all_events(&flt);

    if (setsockopt(dd, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0)
    {
        perror("HCI filter setup failed");
        exit(EXIT_FAILURE);
    }
    return dd;
}

static void send_cmd(int dd, uint8_t ogf, uint16_t ocf, uint8_t *cmd_data, int length)
{
    if (hci_send_cmd(dd, ogf, ocf, length, cmd_data) < 0)
        exit(EXIT_FAILURE);

    unsigned char buf[HCI_MAX_EVENT_SIZE] = {0}, *ptr;
    uint16_t opcode = htobs(cmd_opcode_pack(ogf, ocf));
    hci_event_hdr *hdr = (void *)(buf + 1);
    evt_cmd_complete *cc;
    ssize_t len;
    while ((len = read(dd, buf, sizeof(buf))) < 0)
    {
        if (errno == EAGAIN || errno == EINTR)
            continue;
        printf("While loop for reading event failed\n");
        return;
    }

    hdr = (void *)(buf + 1);
    ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
    len -= (1 + HCI_EVENT_HDR_SIZE);

    switch (hdr->evt)
    {
    case EVT_CMD_COMPLETE:
        cc = (void *)ptr;

        if (cc->opcode != opcode)
            printf("Received event with invalid opcode 0x%X. Expected 0x%X\n", cc->opcode, opcode);

        ptr += EVT_CMD_COMPLETE_SIZE;
        len -= EVT_CMD_COMPLETE_SIZE;

        uint8_t rparam[10] = {0};
        memcpy(rparam, ptr, len);
        if (rparam[0] && ocf != 0x3C)
            printf("Command 0x%X returned error 0x%X\n", ocf, rparam[0]);
        /* if (ocf == OCF_LE_READ_LOCAL_SUPPORTED_FEATURES)
        {
            printf("Supported Low Energy Bluetooth features:\n");
            print_bt_le_features(&rparam[1]);
        } */
        if (ocf == 0x36)
            printf("The transmit power is set to %d dBm\n", (unsigned char)rparam[1]);
        fflush(stdout);
        return;

    default:
        printf("Received unknown event: 0x%X\n", hdr->evt);
        return;
    }
}

static void generate_random_mac_address(uint8_t *mac)
{
    if (!mac)
        return;
    srand(time(0)); // NOLINT(cert-msc51-cpp)
    for (int i = 0; i < 6; i++)
        mac[i] = rand() % 255; // NOLINT(cert-msc50-cpp)

    mac[0] |= 0xC0; // set to Bluetooth Random Static Address, see https://www.novelbits.io/bluetooth-address-privacy-ble/ or section 1.3 of the Bluetooth specification (5.3)
}

static void hci_reset(int dd)
{
    uint8_t ogf = OGF_HOST_CTL; // Opcode Group Field. LE Controller Commands
    uint16_t ocf = OCF_RESET;
    send_cmd(dd, ogf, ocf, NULL, 0);
}

static void hci_le_read_local_supported_features(int dd)
{
    uint8_t ogf = OGF_LE_CTL; // Opcode Group Field. LE Controller Commands
    uint16_t ocf = OCF_LE_READ_LOCAL_SUPPORTED_FEATURES;
    send_cmd(dd, ogf, ocf, NULL, 0);
}

static void hci_le_set_random_address(int dd, const uint8_t *mac)
{
    if (!mac)
        return;
    uint8_t ogf = OGF_LE_CTL; // Opcode Group Field. LE Controller Commands
    uint16_t ocf = OCF_LE_SET_RANDOM_ADDRESS;
    uint8_t buf[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Random_Address:
    for (int i = 0; i < 6; i++)
        buf[i] = mac[i];
    send_cmd(dd, ogf, ocf, buf, sizeof(buf));
}

static void hci_le_set_advertising_enable(int dd)
{
    uint8_t ogf = OGF_LE_CTL; // Opcode Group Field. LE Controller Commands
    uint16_t ocf = OCF_LE_SET_ADVERTISE_ENABLE;
    uint8_t buf[] = {0x01}; // Enable: 0x01 = 1 = Advertising is enabled

    send_cmd(dd, ogf, ocf, buf, sizeof(buf));
}

static void hci_le_set_advertising_disable(int dd)
{
    uint8_t ogf = OGF_LE_CTL; // Opcode Group Field. LE Controller Commands
    uint16_t ocf = OCF_LE_SET_ADVERTISE_ENABLE;
    uint8_t buf[] = {0x00}; // Enable: 0 = Advertising is disabled (default)

    send_cmd(dd, ogf, ocf, buf, sizeof(buf));
}

static void hci_le_set_advertising_set_random_address(int dd, uint8_t set, const uint8_t *mac)
{
    if (!mac)
        return;
    uint8_t ogf = OGF_LE_CTL;                             // Opcode Group Field. LE Controller Commands
    uint16_t ocf = 0x35;                                  // Opcode Command Field: LE Set Advertising Set Random Address
    uint8_t buf[] = {0x00,                                // Advertising_Handle: Used to identify an advertising set
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Advertising_Random_Address:
    buf[0] = set;
    for (int i = 0; i < 6; i++)
        buf[i + 1] = mac[i];
    send_cmd(dd, ogf, ocf, buf, sizeof(buf));
}

static void hci_le_set_advertising_parameters(int dd, int interval_ms)
{
    uint8_t ogf = OGF_LE_CTL; // Opcode Group Field. LE Controller Commands
    uint16_t ocf = OCF_LE_SET_ADVERTISING_PARAMETERS;
    uint8_t buf[] = {0x00, 0x08,                         // Advertising_Interval_Min: N * 0.625 ms. 0x000800 = 1280 ms
                     0x00, 0x08,                         // Advertising_Interval_Max: N * 0.625 ms. 0x000800 = 1280 ms
                     0x03,                               // Advertising_Type: 3 = Non connectable undirected advertising (ADV_NONCONN_IND)
                     0x01,                               // Own_Address_Type: 1 = Random Device Address
                     0x00,                               // Peer_Address_Type: 0 = Public Device Address (default) or Public Identity Address
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Peer_Address
                     0x07,                               // Advertising_Channel_Map: 7 = all three channels enabled
                     0x00};                              // Advertising_Filter_Policy: 0 = Process scan and connection requests from all devices (i.e., the White List is not in use) (default).

    interval_ms = MIN(MAX((1000 * interval_ms) / 625, 0x0020), 0x4000);
    buf[0] = buf[2] = interval_ms & 0xFF;
    buf[1] = buf[3] = (interval_ms >> 8) & 0xFF;

    send_cmd(dd, ogf, ocf, buf, sizeof(buf));
}

static void hci_le_set_advertising_data(int dd)
{
    uint8_t ogf = OGF_LE_CTL; // Opcode Group Field. LE Controller Commands
    uint16_t ocf = OCF_LE_SET_ADVERTISING_DATA;
    uint8_t buf[] = {0x1F,       // Advertising_Data_Length: The number of significant octets in the Advertising_Data.
                     0x1E,       // Length of the service data element
                     0x16,       // 16 = GAP AD Type = "Service Data - 16-bit UUID"
                     0xFA, 0xFF, // 0xFFFA = ASTM International, ASTM Remote ID
                     0x0D,       // 0x0D = AD Application Code within the ASTM address space = Open Drone ID
                     0x00,       // xx = 8-bit message counter starting at 0x00 and wrapping around at 0xFF
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // 25-byte Drone ID message data
    /* buf[6] = msg_counter;
    for (int i = 0; i < ODID_MESSAGE_SIZE; i++)
        buf[7 + i] = encoded->rawData[i]; */

    send_cmd(dd, ogf, ocf, buf, sizeof(buf));
}

static void hci_le_remove_advertising_set(int dd, uint8_t set)
{
    uint8_t ogf = OGF_LE_CTL; // Opcode Group Field. LE Controller Commands
    uint16_t ocf = 0x3C;      // Opcode Command Field: LE Remove Advertising Set
    uint8_t buf[] = {0x00};   // Advertising_Handle: Used to identify an advertising set
    buf[0] = set;
    send_cmd(dd, ogf, ocf, buf, sizeof(buf));
}

int main()
{

    uint8_t mac[6] = {0};
    generate_random_mac_address(mac);

    struct ODID_UAS_Data uasData;
    odid_initUasData(&uasData);
    fill_example_data(&uasData);

    device_descriptor = open_hci_device();
    hci_reset(device_descriptor);
    //Parar transmissao LE
    hci_le_set_advertising_disable(device_descriptor);

    hci_le_read_local_supported_features(device_descriptor);

    // Configura o LE advertise
    hci_reset(device_descriptor);
    hci_le_set_advertising_parameters(device_descriptor, 100);
    hci_le_set_random_address(device_descriptor, mac);

    //Inicia o advertise LE
    hci_le_set_advertising_enable(device_descriptor);

    while (1) 
    {
        hci_le_set_advertising_data(device_descriptor);
    }

    return 0;
}