#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/resource.h>
#include "gpsmod.c"

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "opendroneid.h"
#include "opendroneid.c"
#include "bluetooth.h"
#include "print_bt_features.h"

sem_t semaphore;
pthread_t id, gps_thread;

#define MINIMUM(a, b) (((a) < (b)) ? (a) : (b))

#define BASIC_ID_POS_ZERO 0
#define BASIC_ID_POS_ONE 1

#define ODID_MESSAGE_SIZE 25

int device_descriptor = 0;

static struct config_data config = {0};
static struct fixsource_t source;
static struct gps_data_t gpsdata;

struct gps_loop_args
{
    struct gps_data_t *gpsdata;
    struct ODID_UAS_Data *uasData;
    int exit_status;
};

static void fill_example_data(struct ODID_UAS_Data *uasData)
{
    uasData->BasicID[BASIC_ID_POS_ZERO].UAType = ODID_UATYPE_HELICOPTER_OR_MULTIROTOR;
    uasData->BasicID[BASIC_ID_POS_ZERO].IDType = ODID_IDTYPE_SERIAL_NUMBER;
    char uas_id[] = "555555555555555555AB";
    strncpy(uasData->BasicID[BASIC_ID_POS_ZERO].UASID, uas_id, MINIMUM(sizeof(uas_id), sizeof(uasData->BasicID[BASIC_ID_POS_ZERO].UASID)));

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
    memcpy(uasData->Auth[0].AuthData, auth0_data, MINIMUM(sizeof(auth0_data), sizeof(uasData->Auth[0].AuthData)));

    uasData->Auth[1].AuthType = ODID_AUTH_UAS_ID_SIGNATURE;
    uasData->Auth[1].DataPage = 1;
    char auth1_data[] = "36289078124690153452713";
    memcpy(uasData->Auth[1].AuthData, auth1_data, MINIMUM(sizeof(auth1_data), sizeof(uasData->Auth[1].AuthData)));

    uasData->Auth[2].AuthType = ODID_AUTH_UAS_ID_SIGNATURE;
    uasData->Auth[2].DataPage = 2;
    char auth2_data[] = "12345678901234567890123";
    memcpy(uasData->Auth[2].AuthData, auth2_data, MINIMUM(sizeof(auth2_data), sizeof(uasData->Auth[2].AuthData)));

    uasData->SelfID.DescType = ODID_DESC_TYPE_TEXT;
    char description[] = "Drone de TESTE ID";
    strncpy(uasData->SelfID.Desc, description, MINIMUM(sizeof(description), sizeof(uasData->SelfID.Desc)));

    uasData->System.OperatorLocationType = ODID_OPERATOR_LOCATION_TYPE_TAKEOFF;
    uasData->System.ClassificationType = ODID_CLASSIFICATION_TYPE_EU;
    uasData->System.OperatorLatitude = uasData->Location.Latitude - 23.206495527245156;
    uasData->System.OperatorLongitude = uasData->Location.Longitude - 45.87633407660736; //-23.206495527245156, -45.87633407660736
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

static void fill_example_gps_data(struct ODID_UAS_Data *uasData)
{
    uasData->Location.Status = ODID_STATUS_AIRBORNE;
    uasData->Location.Direction = 361.f;
    uasData->Location.SpeedHorizontal = 0.0f;
    uasData->Location.SpeedVertical = 0.35f;
    uasData->Location.Latitude = -23.20699;
    uasData->Location.Longitude = -45.87736;
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

static void cleanup(int exit_code)
{
    hci_close_dev(device_descriptor);
    if (config.use_gps)
    {
        int *ptr;
        pthread_join(gps_thread, (void **)&ptr);
        printf("Return value from gps_loop: %d\n", *ptr);

        gps_close(&gpsdata);
    }

    exit(exit_code);
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

//Lendo possibilidades do adaptador atual
struct bitfield_data {
    uint64_t bit;
    const char *str;
};
static const struct bitfield_data features_le[] = {
        {  0, "LE Encryption"					},
        {  1, "Connection Parameter Request Procedure"		},
        {  2, "Extended Reject Indication"			},
        {  3, "Slave-initiated Features Exchange"		},
        {  4, "LE Ping"						},
        {  5, "LE Data Packet Length Extension"			},
        {  6, "LL Privacy"					},
        {  7, "Extended Scanner Filter Policies"		},
        {  8, "LE 2M PHY"					},
        {  9, "Stable Modulation Index - Transmitter"		},
        { 10, "Stable Modulation Index - Receiver"		},
        { 11, "LE Coded PHY"					},
        { 12, "LE Extended Advertising"				},
        { 13, "LE Periodic Advertising"				},
        { 14, "Channel Selection Algorithm #2"			},
        { 15, "LE Power Class 1"				},
        { 16, "Minimum Number of Used Channels Procedure"	},
        { 17, "Connection CTE Request"				},
        { 18, "Connection CTE Response"				},
        { 19, "Connectionless CTE Transmitter"			},
        { 20, "Connectionless CTE Receiver"			},
        { 21, "Antenna Switching During CTE Transmission (AoD)"	},
        { 22, "Antenna Switching During CTE Reception (AoA)"	},
        { 23, "Receiving Constant Tone Extensions"		},
        { 24, "Periodic Advertising Sync Transfer - Sender"	},
        { 25, "Periodic Advertising Sync Transfer - Recipient"	},
        { 26, "Sleep Clock Accuracy Updates"			},
        { 27, "Remote Public Key Validation"			},
        { 28, "Connected Isochronous Stream - Master"		},
        { 29, "Connected Isochronous Stream - Slave"		},
        { 30, "Isochronous Broadcaster"				},
        { 31, "Synchronized Receiver"				},
        { 32, "Isochronous Channels (Host Support)"		},
};

#define COLOR_OFF	"\x1B[0m"
#define COLOR_WHITE_BG	"\x1B[0;47;30m"
#define COLOR_UNKNOWN_FEATURE_BIT	COLOR_WHITE_BG

#define print_text(color, fmt, args...) \
		print_indent(8, COLOR_OFF, "", "", color, fmt, ## args)

#define print_field(fmt, args...) \
		print_indent(8, COLOR_OFF, "", "", COLOR_OFF, fmt, ## args)

static pid_t pager_pid = 0;

bool use_color(void)
{
    static int cached_use_color = -1;

    if (__builtin_expect(!!(cached_use_color < 0), 0))
        cached_use_color = isatty(STDOUT_FILENO) > 0 || pager_pid > 0;

    return cached_use_color;
}

#define print_indent(indent, color1, prefix, title, color2, fmt, args...) \
do { \
	printf("%*c%s%s%s%s" fmt "%s\n", (indent), ' ', \
		use_color() ? (color1) : "", prefix, title, \
		use_color() ? (color2) : "", ## args, \
		use_color() ? COLOR_OFF : ""); \
} while (0)

static inline uint64_t print_bitfield(int indent, uint64_t val, const struct bitfield_data *table)
{
    uint64_t mask = val;
    for (int i = 0; table[i].str; i++) {
        if (val & (((uint64_t) 1) << table[i].bit)) {
            print_field("%*c%s", indent, ' ', table[i].str);
            mask &= ~(((uint64_t) 1) << table[i].bit);
        }
    }
    return mask;
}

void print_bt_le_features(const uint8_t *features_array)
{
    uint64_t mask, features = 0;
    char str[41];

    for (int i = 0; i < 8; i++) {
        sprintf(str + (i * 5), " 0x%2.2x", features_array[i]);
        features |= ((uint64_t) features_array[i]) << (i * 8);
    }
    print_field("Features:%s", str);

    mask = print_bitfield(2, features, features_le);
    if (mask)
        print_text("\x1B[0;47;30m", "  Unknown features (0x%16.16" PRIx64 ")", mask);
}

//Envia comandos de baixo nivel
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
        if (ocf == OCF_LE_READ_LOCAL_SUPPORTED_FEATURES)
        {
            printf("Supported Low Energy Bluetooth features:\n");
            print_bt_le_features(&rparam[1]);
        }  
        if (ocf == 0x36)
            printf("The transmit power is set to %d dBm\n", (unsigned char)rparam[1]);
        fflush(stdout);
        return;

    default:
        printf("Received unknown event: 0x%X\n", hdr->evt);
        return;
    }
}

//Mostra o MAC atual no terminal.
void printMACAddress(const uint8_t *mac)
{
    printf("%02X:%02X:%02X:%02X:%02X:%02X\n", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
}

//Gera o MAC atual aleatóriamente.
static void generate_random_mac_address(uint8_t *mac)
{
    if (!mac)
        return;
    srand(time(0)); // NOLINT(cert-msc51-cpp)
    for (int i = 0; i < 6; i++)
        mac[i] = rand() % 255; // NOLINT(cert-msc50-cpp)

    mac[0] |= 0xC0; // set to Bluetooth Random Static Address, see https://www.novelbits.io/bluetooth-address-privacy-ble/ or section 1.3 of the Bluetooth specification (5.3)
    printMACAddress(mac);
}

//Reseta o adaptador
static void hci_reset(int dd)
{
    uint8_t ogf = OGF_HOST_CTL; // Opcode Group Field. LE Controller Commands
    uint16_t ocf = OCF_RESET;
    send_cmd(dd, ogf, ocf, NULL, 0);
}

//Lê as features suportadas do adaptador local
static void hci_le_read_local_supported_features(int dd)
{
    uint8_t ogf = OGF_LE_CTL; // Opcode Group Field. LE Controller Commands
    uint16_t ocf = OCF_LE_READ_LOCAL_SUPPORTED_FEATURES;
    send_cmd(dd, ogf, ocf, NULL, 0);
}

// Max two advertising sets is supported by this function currently
static void hci_le_set_extended_advertising_enable(int dd, struct config_data *config)
{
    uint8_t ogf = OGF_LE_CTL;     // Opcode Group Field. LE Controller Commands
    uint16_t ocf = 0x39;          // Opcode Command Field: LE Set Extended Advertising Enable
    uint8_t buf[] = {0x01,        // Enable: 1 = Advertising is enabled
                     0x01,        // Number_of_Sets: Number of advertising sets to enable or disable
                     0x00, 0x00,  // Advertising_Handle[i]:
                     0x00, 0x00,  // Duration[i]: 0 = No advertising duration. Advertising to continue until the Host disables it
                     0x00, 0x00}; // Max_Extended_Advertising_Events[i]: 0 = No maximum number of advertising events

    if (config->use_bt4 && config->use_bt5)
    {
        buf[1] = 2;
        buf[2] = config->handle_bt4;
        buf[3] = config->handle_bt5;
    }
    else if (config->use_bt4)
    {
        buf[2] = config->handle_bt4;
    }
    else if (config->use_bt5)
    {
        buf[2] = config->handle_bt5;
    }
    send_cmd(dd, ogf, ocf, buf, sizeof(buf));
}

// Desabilita extended advertising
static void hci_le_set_extended_advertising_disable(int dd)
{
    uint8_t ogf = OGF_LE_CTL; // Opcode Group Field. LE Controller Commands
    uint16_t ocf = 0x39;      // Opcode Command Field: LE Set Extended Advertising Enable
    uint8_t buf[] = {0x00,    // Enable: 0 = Advertising is disabled
                     0x00};   // Number_of_Sets: 0 = Disable all advertising sets
    send_cmd(dd, ogf, ocf, buf, sizeof(buf));
}

//Seta entederço aleatório dentro do advertising
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

//Seta os parâmetros do advertising
static void hci_le_set_extended_advertising_parameters(int dd, uint8_t set, int interval_ms, bool long_range)
{
    uint8_t ogf = OGF_LE_CTL;                            // Opcode Group Field. LE Controller Commands
    uint16_t ocf = 0x36;                                 // Opcode Command Field: LE Set Extended Advertising Parameters
    uint8_t buf[] = {0x00,                               // Advertising_Handle: Used to identify an advertising set
                     0x10, 0x00,                         // Advertising_Event_Properties: 0x0010 = Use legacy advertising PDUs + Non-connectable and non-scannable undirected
                     0x00, 0x08, 0x00,                   // Primary_Advertising_Interval_Min: N * 0.625 ms. 0x000800 = 1280 ms
                     0x00, 0x08, 0x00,                   // Primary_Advertising_Interval_Max: N * 0.625 ms. 0x000800 = 1280 ms
                     0x07,                               // Primary_Advertising_Channel_Map: 7 = all three channels enabled
                     0x01,                               // Own_Address_Type: 1 = Random Device Address
                     0x00,                               // Peer_Address_Type: 0 = Public Device Address or Public Identity Address
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Peer_Address
                     0x00,                               // Advertising_Filter_Policy: 0 = Process scan and connection requests from all devices (i.e., the White List is not in use)
                     0x7F,                               // Advertising_Tx_Power: 0x7F = Host has no preference
                     0x01,                               // Primary_Advertising_PHY: 1 = Primary advertisement PHY is LE 1M
                     0x00,                               // Secondary_Advertising_Max_Skip: 0 = AUX_ADV_IND shall be sent prior to the next advertising event
                     0x01,                               // Secondary_Advertising_PHY: 1 = Secondary advertisement PHY is LE 1M
                     0x00,                               // Advertising_SID: 0 = Value of the Advertising SID subfield in the ADI field of the PDU
                     0x00};                              // Scan_Request_Notification_Enable: 0 = Scan request notifications disabled
    buf[0] = set;

    interval_ms = MIN(MAX((1000 * interval_ms) / 625, 0x000020), 0xFFFFFF);
    buf[3] = buf[6] = interval_ms & 0xFF;
    buf[4] = buf[7] = (interval_ms >> 8) & 0xFF;
    buf[5] = buf[8] = (interval_ms >> 16) & 0xFF;

    if (long_range)
    {
        buf[1] = 0x00;  // Advertising_Event_Properties: 0x0000 = Non-connectable and non-scannable undirected
        buf[20] = 0x03; // Primary_Advertising_PHY: 3 = Primary advertisement PHY is LE Coded
        buf[22] = 0x03; // Secondary_Advertising_PHY: 3 = Secondary advertisement PHY is LE Coded
    }

    send_cmd(dd, ogf, ocf, buf, sizeof(buf));
}

//Seta os dados do advertising
static void hci_le_set_extended_advertising_data(int dd, uint8_t set, const union ODID_Message_encoded *encoded, uint8_t msg_counter)
{
    uint8_t ogf = OGF_LE_CTL;    // Opcode Group Field. LE Controller Commands
    uint16_t ocf = 0x37;         // Opcode Command Field: LE Set Extended Advertising Data
    uint8_t buf[] = {0x00,       // Advertising_Handle: Used to identify an advertising set
                     0x03,       // Operation: 3 = Complete extended advertising data
                     0x01,       // Fragment_Preference: 1 = The Controller should not fragment or should minimize fragmentation of Host advertising data
                     0x1F,       // Advertising_Data_Length: The number of octets in the Advertising Data parameter
                     0x1E,       // The length of the following data field
                     0x16,       // 16 = GAP AD Type = "Service Data - 16-bit UUID"
                     0xFA, 0xFF, // 0xFFFA = ASTM International, ASTM Remote ID
                     0x0D,       // 0x0D = AD Application Code within the ASTM address space = Open Drone ID
                     0x00,       // xx = 8-bit message counter starting at 0x00 and wrapping around at 0xFF
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // 25-byte Drone ID message data
    buf[0] = set;
    buf[9] = msg_counter;
    for (int i = 0; i < ODID_MESSAGE_SIZE; i++)
        buf[10 + i] = encoded->rawData[i];

    send_cmd(dd, ogf, ocf, buf, sizeof(buf));
}

//Seta pacote de dados do advertising
static void hci_le_set_extended_advertising_data_pack(int dd, uint8_t set, const struct ODID_MessagePack_encoded *pack_enc, uint8_t msg_counter)
{
    uint8_t ogf = OGF_LE_CTL; // Opcode Group Field. LE Controller Commands
    uint16_t ocf = 0x37;      // Opcode Command Field: LE Set Extended Advertising Data
    uint8_t buf[10 + 3 + ODID_PACK_MAX_MESSAGES * ODID_MESSAGE_SIZE] =
        {0x00,       // Advertising_Handle: Used to identify an advertising set
         0x03,       // Operation: 3 = Complete extended advertising data
         0x01,       // Fragment_Preference: 1 = The Controller should not fragment or should minimize fragmentation of Host advertising data
         0x1F,       // Advertising_Data_Length: The number of octets in the Advertising Data parameter
         0x1E,       // The length of the following data field
         0x16,       // 16 = GAP AD Type = "Service Data - 16-bit UUID"
         0xFA, 0xFF, // 0xFFFA = ASTM International, ASTM Remote ID
         0x0D,       // 0x0D = AD Application Code within the ASTM address space = Open Drone ID
         0x00,       // xx = 8-bit message counter starting at 0x00 and wrapping around at 0xFF
         0x00};      // 3 + ODID_PACK_MAX_MESSAGES*ODID_MESSAGE_SIZE byte Drone ID message pack data
    buf[0] = set;
    buf[9] = msg_counter;

    int amount = pack_enc->MsgPackSize;
    buf[3] = 1 + 5 + 3 + amount * ODID_MESSAGE_SIZE;
    buf[4] = 5 + 3 + amount * ODID_MESSAGE_SIZE;

    for (int i = 0; i < 3 + amount * ODID_MESSAGE_SIZE; i++)
        buf[10 + i] = ((char *)pack_enc)[i];

    send_cmd(dd, ogf, ocf, buf, sizeof(buf));
}

//Envia mensagem
void send_message(const union ODID_Message_encoded *encoded, struct config_data *config, uint8_t msg_counter)
{
    hci_le_set_extended_advertising_data(device_descriptor, config->handle_bt5, encoded, msg_counter);
}

//Envia mensagem única
static void send_single_messages(struct ODID_UAS_Data *uasData, struct config_data *config)
{
    union ODID_Message_encoded encoded;
    memset(&encoded, 0, sizeof(union ODID_Message_encoded));

    for (int i = 0; i < 1; i++)
    {
        if (encodeBasicIDMessage((ODID_BasicID_encoded *)&encoded, &uasData->BasicID[BASIC_ID_POS_ZERO]) != ODID_SUCCESS)
            printf("Error: Failed to encode Basic ID\n");
        send_message(&encoded, config, config->msg_counters[ODID_MSG_COUNTER_BASIC_ID]++);
        if (encodeBasicIDMessage((ODID_BasicID_encoded *)&encoded, &uasData->BasicID[BASIC_ID_POS_ONE]) != ODID_SUCCESS)
            printf("Error: Failed to encode Basic ID\n");
        send_message(&encoded, config, config->msg_counters[ODID_MSG_COUNTER_BASIC_ID]++);

        if (encodeLocationMessage((ODID_Location_encoded *)&encoded, &uasData->Location) != ODID_SUCCESS)
            printf("Error: Failed to encode Location\n");
        send_message(&encoded, config, config->msg_counters[ODID_MSG_COUNTER_LOCATION]++);

        if (encodeAuthMessage((ODID_Auth_encoded *)&encoded, &uasData->Auth[0]) != ODID_SUCCESS)
            printf("Error: Failed to encode Auth 0\n");
        send_message(&encoded, config, config->msg_counters[ODID_MSG_COUNTER_AUTH]++);
        if (encodeAuthMessage((ODID_Auth_encoded *)&encoded, &uasData->Auth[1]) != ODID_SUCCESS)
            printf("Error: Failed to encode Auth 1\n");
        send_message(&encoded, config, config->msg_counters[ODID_MSG_COUNTER_AUTH]++);
        if (encodeAuthMessage((ODID_Auth_encoded *)&encoded, &uasData->Auth[2]) != ODID_SUCCESS)
            printf("Error: Failed to encode Auth 2\n");
        send_message(&encoded, config, config->msg_counters[ODID_MSG_COUNTER_AUTH]++);

        if (encodeSelfIDMessage((ODID_SelfID_encoded *)&encoded, &uasData->SelfID) != ODID_SUCCESS)
            printf("Error: Failed to encode Self ID\n");
        send_message(&encoded, config, config->msg_counters[ODID_MSG_COUNTER_SELF_ID]++);

        if (encodeSystemMessage((ODID_System_encoded *)&encoded, &uasData->System) != ODID_SUCCESS)
            printf("Error: Failed to encode System\n");
        send_message(&encoded, config, config->msg_counters[ODID_MSG_COUNTER_SYSTEM]++);

        if (encodeOperatorIDMessage((ODID_OperatorID_encoded *)&encoded, &uasData->OperatorID) != ODID_SUCCESS)
            printf("Error: Failed to encode Operator ID\n");
        send_message(&encoded, config, config->msg_counters[ODID_MSG_COUNTER_OPERATOR_ID]++);
    }
}

//Cria pacote de mensagem
static void create_message_pack(struct ODID_UAS_Data *uasData, struct ODID_MessagePack_encoded *pack_enc)
{
    union ODID_Message_encoded encoded = {0};
    ODID_MessagePack_data pack_data = {0};
    pack_data.SingleMessageSize = ODID_MESSAGE_SIZE;
    pack_data.MsgPackSize = 9;
    if (encodeBasicIDMessage((ODID_BasicID_encoded *)&encoded, &uasData->BasicID[BASIC_ID_POS_ZERO]) != ODID_SUCCESS)
        printf("Error: Failed to encode Basic ID\n");
    memcpy(&pack_data.Messages[0], &encoded, ODID_MESSAGE_SIZE);
    if (encodeBasicIDMessage((ODID_BasicID_encoded *)&encoded, &uasData->BasicID[BASIC_ID_POS_ONE]) != ODID_SUCCESS)
        printf("Error: Failed to encode Basic ID\n");
    memcpy(&pack_data.Messages[1], &encoded, ODID_MESSAGE_SIZE);
    if (encodeLocationMessage((ODID_Location_encoded *)&encoded, &uasData->Location) != ODID_SUCCESS)
        printf("Error: Failed to encode Location\n");
    memcpy(&pack_data.Messages[2], &encoded, ODID_MESSAGE_SIZE);
    if (encodeAuthMessage((ODID_Auth_encoded *)&encoded, &uasData->Auth[0]) != ODID_SUCCESS)
        printf("Error: Failed to encode Auth 0\n");
    memcpy(&pack_data.Messages[3], &encoded, ODID_MESSAGE_SIZE);
    if (encodeAuthMessage((ODID_Auth_encoded *)&encoded, &uasData->Auth[1]) != ODID_SUCCESS)
        printf("Error: Failed to encode Auth 1\n");
    memcpy(&pack_data.Messages[4], &encoded, ODID_MESSAGE_SIZE);
    if (encodeAuthMessage((ODID_Auth_encoded *)&encoded, &uasData->Auth[2]) != ODID_SUCCESS)
        printf("Error: Failed to encode Auth 2\n");
    memcpy(&pack_data.Messages[5], &encoded, ODID_MESSAGE_SIZE);
    if (encodeSelfIDMessage((ODID_SelfID_encoded *)&encoded, &uasData->SelfID) != ODID_SUCCESS)
        printf("Error: Failed to encode Self ID\n");
    memcpy(&pack_data.Messages[6], &encoded, ODID_MESSAGE_SIZE);
    if (encodeSystemMessage((ODID_System_encoded *)&encoded, &uasData->System) != ODID_SUCCESS)
        printf("Error: Failed to encode System\n");
    memcpy(&pack_data.Messages[7], &encoded, ODID_MESSAGE_SIZE);
    if (encodeOperatorIDMessage((ODID_OperatorID_encoded *)&encoded, &uasData->OperatorID) != ODID_SUCCESS)
        printf("Error: Failed to encode Operator ID\n");
    memcpy(&pack_data.Messages[8], &encoded, ODID_MESSAGE_SIZE);
    if (encodeMessagePack(pack_enc, &pack_data) != ODID_SUCCESS)
        printf("Error: Failed to encode message pack_data\n");
}

//Envia pacote de mensagem
void send_bluetooth_message_pack(const struct ODID_MessagePack_encoded *pack_enc, uint8_t msg_counter, struct config_data *config)
{
    hci_le_set_extended_advertising_data_pack(device_descriptor, config->handle_bt5, pack_enc, msg_counter);
}

//Envia pacotes
static void send_packs(struct ODID_UAS_Data *uasData, struct config_data *config)
{
    struct ODID_MessagePack_encoded pack_enc = {0};
    create_message_pack(uasData, &pack_enc);

    for (int i = 0; i < 10; i++)
    {
        send_bluetooth_message_pack(&pack_enc, config->msg_counters[ODID_MSG_COUNTER_PACKED]++, config);
        sleep(4);
    }
}


//Remove o advertising
static void hci_le_remove_advertising_set(int dd, uint8_t set)
{
    uint8_t ogf = OGF_LE_CTL; // Opcode Group Field. LE Controller Commands
    uint16_t ocf = 0x3C;      // Opcode Command Field: LE Remove Advertising Set
    uint8_t buf[] = {0x00};   // Advertising_Handle: Used to identify an advertising set
    buf[0] = set;
    send_cmd(dd, ogf, ocf, buf, sizeof(buf));
}

//Para a transmissão
static void stop_transmit(struct config_data *config)
{
  hci_le_set_extended_advertising_disable(device_descriptor);
  hci_le_remove_advertising_set(device_descriptor, config->handle_bt5);
}

//Inicia bluetooth e parametros do advertising
void init_bluetooth(struct config_data *config)
{
    uint8_t mac[6] = {0};
    generate_random_mac_address(mac);

    device_descriptor = open_hci_device();
    hci_reset(device_descriptor);
    stop_transmit(config);

    hci_le_read_local_supported_features(device_descriptor);

    hci_reset(device_descriptor);
    hci_le_set_extended_advertising_parameters(device_descriptor, config->handle_bt5, 950, true);
    hci_le_set_advertising_set_random_address(device_descriptor, config->handle_bt5, mac);

    hci_le_set_extended_advertising_enable(device_descriptor, config);
}

//Loop de gps
void gps_loop(struct gps_loop_args *args)
{
    struct gps_data_t *gpsdata = args->gpsdata;
    struct ODID_UAS_Data *uasData = args->uasData;

    char gpsd_message[GPS_JSON_RESPONSE_MAX];
    int retries = 0; // cycles to wait before gpsd timeout
    int read_retries = 0;
    /* while (true)
    {
        int ret;
        ret = gps_waiting(gpsdata, GPS_WAIT_TIME_MICROSECS);
        if (!ret)
        {
            printf("Socket not ready, retrying...\n");
            if (retries++ > MAX_GPS_WAIT_RETRIES)
            {
                fprintf(stderr, "Max socket wait retries reached, exiting...");
                args->exit_status = 1;
                pthread_exit((void *)&args->exit_status);
            }
            continue;
        }
        else
        {
            retries = 0;
            gpsd_message[0] = '\0';

            if (gps_read(gpsdata, gpsd_message, sizeof(gpsd_message)) == -1)
            {
                printf("Failed to read from socket, retrying...\n");
                if (read_retries++ > MAX_GPS_READ_RETRIES)
                {
                    fprintf(stderr, "Max socket read retries reached, exiting...");

                    args->exit_status = 1;
                    pthread_exit((void *)&args->exit_status);
                }
                continue;
            }
            read_retries = 0;

        }
    }
 */
    /*process_gps_data(gpsdata, uasData);
     args->exit_status = 0;
     pthread_exit(&args->exit_status); */
}

int main()
{
    config.handle_bt5 = 1; // The Extended Advertising set number used for BT5
    config.use_packs = true;

    struct ODID_UAS_Data uasData;
    odid_initUasData(&uasData);
    fill_example_data(&uasData);
    fill_example_gps_data(&uasData);

    // Parar transmissao LE
    hci_le_set_extended_advertising_disable(device_descriptor);

    init_bluetooth(&config);

    if (init_gps(&source, &gpsdata) != 0)
    {
        fprintf(stderr,
                "No gpsd running or network error: %d, %s\n",
                errno, gps_errstr(errno));
        cleanup(EXIT_FAILURE);
    }

    struct gps_loop_args args;
    args.gpsdata = &gpsdata;
    args.uasData = &uasData;
    pthread_create(&gps_thread, NULL, (void *)&gps_loop, &args);

    if (config.use_packs)
        printf("Enviando pacotes.");
    else
        printf("Enviando single message");

    while (true)
    {
        if (config.use_packs)
        {
            send_packs(&uasData, &config);
        }
        else
        {
            send_single_messages(&uasData, &config);
        }
    }

    stop_transmit(&config);

    return 0;
}