#include <errno.h>

#include "../include/gpsmod.h"

#include "advle.h"
#include "scan.h"



bool kill_program = false;

static void parse_command_line(int argc, char *argv[], struct config_data *config)
{
        if (argc == 1)
        {
                exit(EXIT_SUCCESS);
        }

        for (int i = 1; i < argc; i++)
        {
                switch (*argv[i])
                {
                case 'b':
                        config->use_beacon = true;
                        break;
                case 'l':
                        config->use_btl = true;
                        break;
                case '4':
                        config->use_bt4 = true;
                        break;
                case '5':
                        config->use_bt5 = true;
                        break;
                case 'p':
                        config->use_packs = true;
                        break;
                case 'g':
                        config->use_gps = true;
                        break;
                default:
                        break;
                }
        }
}
static void sig_handler_main(int signo)
{
        if (signo == SIGINT || signo == SIGSTOP || signo == SIGKILL || signo == SIGTERM)
        {
                kill_program = true;
        }
}
int main(int argc, char *argv[])
{
        parse_command_line(argc, argv, &config);
        odid_initUasData(&uasData);
        fill_example_data(&uasData, &config);
        fill_example_gps_data(&uasData);

        // Setting bluetooth for advertising
        init_bluetooth();
	const int device = open_hci_device();
        init_scan(device);

        signal(SIGINT, sig_handler_main);
        signal(SIGKILL, sig_handler_main);
        signal(SIGSTOP, sig_handler_main);
        signal(SIGTERM, sig_handler_main);
        int i = 0;

        if (config.use_gps) // Enables GPS usage and thread.
        {
                printf("GPS: LIGADO\n");
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

                int ret = pthread_create(&gps_thread, NULL, (void *)&gps_thread_function, &args);
                if (ret != 0)
                {
                        fprintf(stderr, "Falha ao criar a pthread.\n");
                        return 1;
                }

                while (true)
                {
                        if (kill_program)
                                break;
                        advertise_le();
                }
        }
        else
        {
                printf("GPS: DESLIGADO\n");
                while (true)
                {
                        if (kill_program)
                                break;
                        advertise_le();
                        hci_reset(device);
                        scan_le();
                }
        }

        cleanup(EXIT_SUCCESS);
}