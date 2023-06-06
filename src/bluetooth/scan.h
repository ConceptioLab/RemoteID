#include <stdint.h>
#include <sys/types.h>

#include "../include/opendroneid.h"

#define MAX_UAVS    20
#define ID_OD_AUTH_DATUM  1546300800LU
typedef __u_char u_char;

struct UAV_RID {u_char            mac[6], counter[16];
                unsigned int      packets;
                time_t            last_rx, last_retx;
                ODID_UAS_Data     odid_data;
                ODID_BasicID_data basic_serial, basic_caa_reg;

                int               rssi;
                double            min_lat, max_lat, min_long, max_long, min_alt, max_alt;
};

void  parse_odid(u_char *,u_char *,int,int,const char *,const float *);
int   mac_index(uint8_t *,struct UAV_RID *);
void  dump(char *,uint8_t *,int);
char *printable_text(uint8_t *,int);
