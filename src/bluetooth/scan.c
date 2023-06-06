#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <ctype.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "../opendroneid.h"
#include "../opendroneid.c"
#include "scan.h"

#define DEBUG_FILE 1

static bool kill_program = false;
static int sniffer = -1, json_socket = -1, max_udp_length = 0;
static uint32_t counter = 1;
static struct UAV_RID RID_data[MAX_UAVS];
static struct sockaddr_in server;

static FILE *debug_file = NULL;
static char *log_dir = NULL;
char text[128];
const mode_t file_mode = 0666, dir_mode = 0777;

static const char default_key[] = "0123456789abcdef",
				  default_iv[] = "nopqrs",
				  default_server[] = "127.0.0.1",
				  debug_filename[] = "debug.txt",
				  device_pi[] = "wlan1",
				  device_i686[] = "wlp5s0b1",
				  default_log_dir[] = "/tmp/rid_capture",
				  default_www_dir[] = "www",
				  dummy[] = "";

static volatile uint32_t rx_packets = 0, odid_packets = 0;

struct hci_request ble_hci_request(uint16_t ocf, int clen, void *status, void *cparam)
{
	struct hci_request rq;
	memset(&rq, 0, sizeof(rq));
	rq.ogf = OGF_LE_CTL;
	rq.ocf = ocf;
	rq.cparam = cparam;
	rq.clen = clen;
	rq.rparam = status;
	rq.rlen = 1;
	return rq;
}

static void sig_handler(int signo)
{
	if (signo == SIGINT || signo == SIGSTOP || signo == SIGKILL || signo == SIGTERM)
	{
		kill_program = true;
	}
}

void parse_odid(u_char *mac, u_char *payload, int length, int rssi, const char *note, const float *volts)
{

	int i, j, RID_index, page, authenticated;
	char json[128];
	uint8_t counter, index;
	double latitude, longitude, altitude;
	ODID_UAS_Data UAS_data;
	ODID_MessagePack_encoded *encoded_data = (ODID_MessagePack_encoded *)&payload[1];

	i = 0;
	authenticated = 0;

	/* */

	RID_index = mac_index(mac, RID_data);

	/* Decode */

	counter = payload[0];
	index = payload[1] >> 4;

#if 1
	if (RID_data[RID_index].counter[index] == counter)
	{
		return;
	}
#endif

	RID_data[RID_index].counter[index] = counter;

	++odid_packets;
	++RID_data[RID_index].packets;
	RID_data[RID_index].rssi = rssi;

#if 0
	if (note)
	{
		display_note(RID_index + 1, note);
	}

	if (volts)
	{
		display_voltage(RID_index + 1, *volts);
	}
#endif
	memset(&UAS_data, 0, sizeof(UAS_data));

	switch (payload[1] & 0xf0)
	{

	case 0x00:
		decodeBasicIDMessage(&UAS_data.BasicID[0], (ODID_BasicID_encoded *)&payload[1]);
		UAS_data.BasicIDValid[0] = 1;
		break;

	case 0x10:
		decodeLocationMessage(&UAS_data.Location, (ODID_Location_encoded *)&payload[1]);
		UAS_data.LocationValid = 1;
		break;

	case 0x20:
		page = payload[2] & 0x0f;
		decodeAuthMessage(&UAS_data.Auth[page], (ODID_Auth_encoded *)&payload[1]);
		UAS_data.AuthValid[page] = 1;
		break;

	case 0x30:
		decodeSelfIDMessage(&UAS_data.SelfID, (ODID_SelfID_encoded *)&payload[1]);
		UAS_data.SelfIDValid = 1;
		break;

	case 0x40:
		decodeSystemMessage(&UAS_data.System, (ODID_System_encoded *)&payload[1]);
		UAS_data.SystemValid = 1;
		break;

	case 0x50:
		decodeOperatorIDMessage(&UAS_data.OperatorID, (ODID_OperatorID_encoded *)&payload[1]);
		UAS_data.OperatorIDValid = 1;
		break;

	case 0xf0:
		decodeMessagePack(&UAS_data, encoded_data);
		break;
	}

	/* JSON */

	sprintf(json, "{ \"mac\" : \"%02x:%02x:%02x:%02x:%02x:%02x\"",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	write_json(json);

#if 1
	sprintf(json, ", \"rssi\" : %d", rssi);
	write_json(json);
#endif

#if 0
  sprintf(json,", \"debug\" : \"%d | ",length);
  //write_json(json);

  for (i = 0; i < 16; ++i) {

    sprintf(json,"%02x ",payload[i]);
    //write_json(json);
  }

  //write_json("\"");
#endif

	if (UAS_data.OperatorIDValid)
	{

		sprintf(json, ", \"operator\" : \"%s\"", UAS_data.OperatorID.OperatorId);
		write_json(json);

		memcpy(&RID_data[RID_index].odid_data.OperatorID, &UAS_data.OperatorID, sizeof(ODID_OperatorID_data));
		// display_identifier(RID_index + 1, UAS_data.OperatorID.OperatorId);
	}

	for (j = 0; j < ODID_BASIC_ID_MAX_MESSAGES; ++j)
	{

		if (UAS_data.BasicIDValid[j])
		{

			memcpy(&RID_data[RID_index].odid_data.BasicID[j], &UAS_data.BasicID[j], sizeof(ODID_BasicID_data));

			switch (UAS_data.BasicID[j].IDType)
			{

			case ODID_IDTYPE_SERIAL_NUMBER:
				memcpy(&RID_data[RID_index].basic_serial, &UAS_data.BasicID[j], sizeof(ODID_BasicID_data));
				sprintf(json, ", \"uav id\" : \"%s\"", UAS_data.BasicID[j].UASID);
				write_json(json);
#if 0
        //display_identifier(RID_index + 1,UAS_data.BasicID[j].UASID);
#endif
				break;

			case ODID_IDTYPE_CAA_REGISTRATION_ID:
				memcpy(&RID_data[RID_index].basic_caa_reg, &UAS_data.BasicID[j], sizeof(ODID_BasicID_data));
				sprintf(json, ", \"caa registration\" : \"%s\"", UAS_data.BasicID[j].UASID);
				write_json(json);
				// display_identifier(RID_index + 1, UAS_data.BasicID[j].UASID);
				break;

			case ODID_IDTYPE_NONE:
			default:
				break;
			}
		}
	}

	if (UAS_data.LocationValid)
	{

		latitude = UAS_data.Location.Latitude;
		longitude = UAS_data.Location.Longitude;
		altitude = UAS_data.Location.AltitudeGeo;

		sprintf(json, ", \"uav latitude\" : %11.6f, \"uav longitude\" : %11.6f",
				latitude, longitude);
		write_json(json);
		sprintf(json, ", \"uav altitude\" : %d, \"uav heading\" : %d",
				(int)UAS_data.Location.AltitudeGeo, (int)UAS_data.Location.Direction);
		write_json(json);
#if 0
    sprintf(json,", \"uav speed horizontal\" : %d, \"uav speed vertical\" : %d",
           (int) UAS_data.Location.SpeedHorizontal,(int) UAS_data.Location.SpeedVertical);
    //write_json(json);
#endif
		sprintf(json, ", \"uav speed\" : %d, \"seconds\" : %d",
				(int)UAS_data.Location.SpeedHorizontal, (int)UAS_data.Location.TimeStamp);
		write_json(json);

		memcpy(&RID_data[RID_index].odid_data.Location, &UAS_data.Location, sizeof(ODID_Location_data));

		// display_uav_loc(RID_index + 1, latitude, longitude, (int)altitude, (int)UAS_data.Location.TimeStamp);

		if ((latitude < RID_data[RID_index].min_lat) || (RID_data[RID_index].min_lat == 0.0))
		{
			RID_data[RID_index].min_lat = latitude;
		}

		if ((latitude > RID_data[RID_index].max_lat) || (RID_data[RID_index].max_lat == 0.0))
		{
			RID_data[RID_index].max_lat = latitude;
		}

		if ((longitude < RID_data[RID_index].min_long) || (RID_data[RID_index].min_long == 0.0))
		{
			RID_data[RID_index].min_long = longitude;
		}

		if ((longitude > RID_data[RID_index].max_long) || (RID_data[RID_index].max_long == 0.0))
		{
			RID_data[RID_index].max_long = longitude;
		}

		if (altitude > INV_ALT)
		{

			if ((altitude < RID_data[RID_index].min_alt) || (RID_data[RID_index].min_alt == 0.0))
			{
				RID_data[RID_index].min_alt = altitude;
			}

			if ((altitude > RID_data[RID_index].max_alt) || (RID_data[RID_index].max_alt == 0.0))
			{
				RID_data[RID_index].max_alt = altitude;
			}
		}
	}

	if (UAS_data.SystemValid)
	{

		sprintf(json, ", \"base latitude\" : %11.6f, \"base longitude\" : %11.6f",
				UAS_data.System.OperatorLatitude, UAS_data.System.OperatorLongitude);
		write_json(json);
		sprintf(json, ", \"unix time\" : %lu",
				((unsigned long int)UAS_data.System.Timestamp) + ID_OD_AUTH_DATUM);
		write_json(json);

		memcpy(&RID_data[RID_index].odid_data.System, &UAS_data.System, sizeof(ODID_System_data));

		// display_timestamp(RID_index + 1, (time_t)UAS_data.System.Timestamp + ID_OD_AUTH_DATUM);
	}

	if (UAS_data.SelfIDValid)
	{

		sprintf(json, ", \"self id\" : \"%s\"", UAS_data.SelfID.Desc);
		write_json(json);

		memcpy(&RID_data[RID_index].odid_data.SelfID, &UAS_data.SelfID, sizeof(ODID_SelfID_data));
	}

	for (page = 0; page < ODID_AUTH_MAX_PAGES; ++page)
	{

		if (UAS_data.AuthValid[page])
		{

			if (page == 0)
			{

				sprintf(json, ", \"unix time (alt)\" : %lu",
						((unsigned long int)UAS_data.Auth[page].Timestamp) + ID_OD_AUTH_DATUM);
				write_json(json);

				// display_timestamp(RID_index + 1, (time_t)UAS_data.Auth[page].Timestamp + ID_OD_AUTH_DATUM);
			}

			sprintf(json, ", \"auth page %d\" : { \"text\" : \"%s\"", page,
					printable_text(UAS_data.Auth[page].AuthData,
								   (page) ? ODID_AUTH_PAGE_NONZERO_DATA_SIZE : ODID_AUTH_PAGE_ZERO_DATA_SIZE));
			write_json(json);

#if 1
			write_json(", \"values\" : [");

			for (i = 0; i < ((page) ? ODID_AUTH_PAGE_NONZERO_DATA_SIZE : ODID_AUTH_PAGE_ZERO_DATA_SIZE); ++i)
			{

				sprintf(json, "%s %d", (i) ? "," : "", UAS_data.Auth[page].AuthData[i]);
				write_json(json);
			}

			write_json(" ]");
#endif
			write_json(" }");

			memcpy(&RID_data[RID_index].odid_data.Auth[page], &UAS_data.Auth[page], sizeof(ODID_Auth_data));
		}
	}

#if VERIFY
	authenticated = parse_auth(&UAS_data, encoded_data, &RID_data[RID_index]);
	// display_pass(RID_index + 1, (authenticated) ? pass_s : "    ");
#endif

	write_json(" }\n");
	printf("%s\n", json);

	/* */

#if 0
  for (i = 0; (i < length)&&(i < 16); ++i) {

    fprintf(stderr,"%02x ",payload[i]);
  }
  
  fprintf(stderr,"%s\n",printable_text(payload,16));
#endif

	return;
}

void advert_odid(evt_le_meta_event *event, int *adverts)
{
	int i, offset = 1;
	char address[18];
	le_advertising_info *advert;
	uint8_t mac[6];

	for (i = 0, offset = 1; i < event->data[0]; ++i)
	{

		memset(mac, 0, sizeof(mac));
		advert = (le_advertising_info *)&event->data[offset];

		ba2str(&advert->bdaddr, address);
		sscanf(address, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX",
			   &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

		const int odid_offset = 5;

		if ((advert->data[4] == 0x0D) &&
			(advert->data[2] == 0xFA) &&
			(advert->data[3] == 0xFF))
		{
			printf("%s\n", address);
			++(*adverts);
			parse_odid(mac, &advert->data[odid_offset], advert->length - odid_offset, 0, "BlueZ", NULL);
		}

		offset += advert->length + 2;
	}
}

int parse_bluez_sniffer()
{
	int adverts = 0, bytes;
	uint8_t buffer[HCI_MAX_EVENT_SIZE];
	evt_le_meta_event *event;

	event = (evt_le_meta_event *)&buffer[HCI_EVENT_HDR_SIZE + 1];

	if ((bytes = read(sniffer, buffer, sizeof(buffer))) > HCI_EVENT_HDR_SIZE)
	{

		if (event->subevent == EVT_LE_ADVERTISING_REPORT)
		{
			advert_odid(event, &adverts);
		}
		++counter;
	}
	return adverts;
}

int main()
{
	int ret, status;
	// display_init();

	// Get HCI device.

	const int device = hci_open_dev(hci_get_route(NULL));
	if (device < 0)
	{
		perror("Failed to open HCI device.");
		return 0;
	}

	// Resetar adaptador.
	hci_send_cmd(device, OGF_HOST_CTL, OCF_RESET, 0, 0);

	// Setar BLE scan parameters.
	le_set_scan_parameters_cp scan_params_cp;
	memset(&scan_params_cp, 0, sizeof(scan_params_cp));
	scan_params_cp.type = 0x00;
	scan_params_cp.interval = htobs(0x0010);
	scan_params_cp.window = htobs(0x0010);
	scan_params_cp.own_bdaddr_type = 0x00; // Public Device Address (default).
	scan_params_cp.filter = 0x00;		   // Accept all.

	struct hci_request scan_params_rq = ble_hci_request(OCF_LE_SET_SCAN_PARAMETERS, LE_SET_SCAN_PARAMETERS_CP_SIZE, &status, &scan_params_cp);

	ret = hci_send_req(device, &scan_params_rq, 1000);
	if (ret < 0)
	{
		hci_close_dev(device);
		perror("Failed to set scan parameters data.");
		return 0;
	}

	// Set BLE events report mask.
	le_set_event_mask_cp event_mask_cp;
	memset(&event_mask_cp, 0, sizeof(le_set_event_mask_cp));
	int i = 0;
	for (i = 0; i < 8; i++)
		event_mask_cp.mask[i] = 0xFF;

	struct hci_request set_mask_rq = ble_hci_request(OCF_LE_SET_EVENT_MASK, LE_SET_EVENT_MASK_CP_SIZE, &status, &event_mask_cp);
	ret = hci_send_req(device, &set_mask_rq, 1000);
	if (ret < 0)
	{
		hci_close_dev(device);
		perror("Failed to set event mask.");
		return 0;
	}

	// Enable scanning.
	le_set_scan_enable_cp scan_cp;
	memset(&scan_cp, 0, sizeof(scan_cp));
	scan_cp.enable = 0x01;	   // Enable flag.
	scan_cp.filter_dup = 0x00; // Filtering disabled.

	struct hci_request enable_adv_rq = ble_hci_request(OCF_LE_SET_SCAN_ENABLE, LE_SET_SCAN_ENABLE_CP_SIZE, &status, &scan_cp);

	ret = hci_send_req(device, &enable_adv_rq, 1000);
	if (ret < 0)
	{
		hci_close_dev(device);
		perror("Failed to enable scan.");
		return 0;
	}

#if DEBUG_FILE
	sprintf(text, "%s/%s", log_dir, debug_filename);
	debug_file = fopen(text, "w");
	chmod(text, file_mode);
#endif

	// Get Results.
	struct hci_filter nf;
	hci_filter_clear(&nf);
	hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
	hci_filter_set_event(EVT_LE_META_EVENT, &nf);
	if (setsockopt(device, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0)
	{
		hci_close_dev(device);
		perror("Could not set socket options\n");
		return 0;
	}

	signal(SIGINT, sig_handler);
	signal(SIGKILL, sig_handler);
	signal(SIGSTOP, sig_handler);
	signal(SIGTERM, sig_handler);

	printf("Scanning....\n");

	uint8_t buf[HCI_MAX_EVENT_SIZE];
	evt_le_meta_event *meta_event;
	le_advertising_info *info;
	int len, page;

	ODID_UAS_Data UAS_data;
	ODID_MessagePack_encoded *encoded_data = (ODID_MessagePack_encoded *)&info->data;

	while (1)
	{
		if (kill_program)
			break;

		parse_bluez_sniffer();
	}

	// Disable scanning.

	memset(&scan_cp, 0, sizeof(scan_cp));
	scan_cp.enable = 0x00; // Disable flag.

	struct hci_request disable_adv_rq = ble_hci_request(OCF_LE_SET_SCAN_ENABLE, LE_SET_SCAN_ENABLE_CP_SIZE, &status, &scan_cp);
	ret = hci_send_req(device, &disable_adv_rq, 1000);
	if (ret < 0)
	{
		hci_close_dev(device);
		perror("Failed to disable scan.");
		return 0;
	}

	hci_close_dev(device);
	// display_end();

	return 0;
}

int write_json(char *json)
{

	int status;
#if UDP_BUFFER_SIZE
	static int index = 0;
	static uint8_t udp_buffer[UDP_BUFFER_SIZE + 2], c;
#else
	int l;
#endif

	if (json_socket > -1)
	{

#if UDP_BUFFER_SIZE
		while (*json)
		{

			udp_buffer[index++] = c = (uint8_t)*json++;

			if ((c == 0x0a) ||
				(c == 0x0d) ||
				(index >= UDP_BUFFER_SIZE))
			{

				udp_buffer[index] = 0;

				if ((status = sendto(json_socket, udp_buffer, index, 0,
									 (struct sockaddr *)&server, sizeof(server))) < 0)
				{

					fprintf(stderr, "%s(): %d, %d, %d\n",
							__func__, index, status, errno);
				}

				if (index > max_udp_length)
				{
					max_udp_length = index;
				}

				index = 0;
				break;
			}
		}
#else
		if ((status = sendto(json_socket, json, l = strlen(json), 0,
							 (struct sockaddr *)&server, sizeof(server))) < 0)
		{

			fprintf(stderr, "%s(): %d, %d, %d\n",
					__func__, l, status, errno);
		}
#endif
	}
	else
	{

		fputs(json, stdout);
	}

	return 0;
}

int mac_index(uint8_t *mac, struct UAV_RID *RID_data)
{

	int i, RID_index = 0, oldest = 0;
	char text[64];
	time_t secs, oldest_secs;

	time(&secs);

	RID_index =
		oldest = 0;
	oldest_secs = secs;

	for (i = 0; i < MAX_UAVS; ++i)
	{

		if (memcmp(mac, RID_data[i].mac, 6) == 0)
		{

			RID_index = i;
			RID_data[i].last_rx = secs;

			break;
		}

		if (RID_data[i].last_rx < oldest_secs)
		{

			oldest = i;
			oldest_secs = RID_data[i].last_rx;
		}
	}

	if (i == MAX_UAVS)
	{

		struct UAV_RID *uav;

		uav = &RID_data[oldest];

		sprintf(text, "%02x:%02x:%02x:%02x:%02x:%02x",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

		// display__mac(oldest + 1, mac);

		fprintf(stderr, " - using RID record %d\n", oldest);

		RID_index = oldest;
		uav->last_rx = secs;
		uav->last_retx = 0;
		uav->packets = 0;

		memcpy(uav->mac, mac, 6);
		memset(&uav->odid_data, 0, sizeof(ODID_UAS_Data));

#if VERIFY
		uav->auth_length = 0;
		memset(uav->auth_buffer, 0, sizeof(uav->auth_buffer));
#endif
	}

	return RID_index;
}

/*
 *
 */

void dump(char *name, uint8_t *data, int len)
{

	int i;

	if (debug_file)
	{

		fprintf(debug_file, "%s[] = {", name);

		for (i = 0; i < len; ++i)
		{

			fprintf(debug_file, "%s 0x%02x", (i) ? "," : "", data[i]);
		}

		fprintf(debug_file, " };\n%s_s = \"%s\";\n", name, printable_text(data, len));

		fflush(debug_file);
	}

	return;
}

/*
 *
 *  \ is 0x5c
 */

char *printable_text(uint8_t *data, int len)
{

	int i;
	static char text[32];

	for (i = 0; (i < 31) && (i < len); ++i)
	{

		text[i] = (isprint(data[i]) && (data[i] != '"') && (data[i] != '\\')) ? (char)data[i] : '.';
	}

	text[i] = 0;

	return text;
}