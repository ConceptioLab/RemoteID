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
#include "../include/gpsmod.c"

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "../include/opendroneid.h"
#include "../include/opendroneid.c"
#include "bluetooth.h"
#include "print_bt_features.h"

#define MINIMUM(a, b) (((a) < (b)) ? (a) : (b))

#define BASIC_ID_POS_ZERO 0
#define BASIC_ID_POS_ONE 1

#define ODID_MESSAGE_SIZE 25

sem_t semaphore;
pthread_t id, gps_thread;

int first = 1;

int device_descriptor = 0;

static struct config_data config = {0};
static bool kill_program = false;

static struct fixsource_t source;
static struct gps_data_t gpsdata;

struct gps_loop_args
{
        struct gps_data_t *gpsdata;
        struct ODID_UAS_Data *uasData;
        int exit_status;
};
struct ODID_UAS_Data uasData;

int get_mac();
void init_bluetooth(struct config_data *config);
void *gps_thread_function(struct gps_loop_args *args);
int advertise_le();
void sig_handler(int signo);