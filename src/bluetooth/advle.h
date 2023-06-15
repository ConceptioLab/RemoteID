
#include "../include/utils.h"
#include <pthread.h>

#ifndef GLOBAL_VARIABLE_H
#define GLOBAL_VARIABLE_H

extern bool kill_program;

#endif


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


int get_mac();
void init_bluetooth(struct config_data *config);
void *gps_thread_function(struct gps_loop_args *args);
void fill_example_data(struct ODID_UAS_Data *uasData);
void cleanup(int exit_code);
void advertise_le();