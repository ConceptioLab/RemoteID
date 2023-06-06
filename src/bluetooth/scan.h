#include <stdint.h>
#include <sys/types.h>

#include "../opendroneid.h"

#define MAX_UAVS    20
typedef __u_char u_char;

struct UAV_RID {u_char            mac[6], counter[16];
                unsigned int      packets;
                time_t            last_rx, last_retx;
                ODID_UAS_Data     odid_data;
                ODID_BasicID_data basic_serial, basic_caa_reg;

                int               rssi;
                double            min_lat, max_lat, min_long, max_long, min_alt, max_alt;
};

#define ID_OD_AUTH_DATUM  1546300800LU
