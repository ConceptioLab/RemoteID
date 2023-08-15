
#ifndef ADVLE_H
#define ADVLE_H
#ifdef __cplusplus
extern "C" {
#endif
#include "../include/utils.h"
#include <pthread.h>

#define MINIMUM(a, b) (((a) < (b)) ? (a) : (b))

#define BASIC_ID_POS_ZERO 0
#define BASIC_ID_POS_ONE 1

#define ODID_MESSAGE_SIZE 25

static struct config_data config = {0};

static struct fixsource_t source;
static struct gps_data_t gpsdata;

extern struct ODID_UAS_Data uasData;
extern pthread_t id, gps_thread;

struct gps_loop_args
{
        struct gps_data_t *gpsdata;
        struct ODID_UAS_Data *uasData;
        int exit_status;
};


extern int open_hci_device();
int get_mac();
extern void init_bluetooth();
void *gps_thread_function(void *args);
void fill_example_data(struct ODID_UAS_Data *uasData, struct config_data *config);
void fill_example_gps_data(struct ODID_UAS_Data *uasData);
void hci_reset(int dd);
void cleanup(int exit_code);
void advertise_le();

#ifdef __cplusplus
}
#endif
#endif
