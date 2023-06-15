#include "advle.h"
#include "scan.h"

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

int main(int argc, char *argv[])
{
        parse_command_line(argc, argv, &config);
        odid_initUasData(&uasData);
        fill_example_data(&uasData);

        init_bluetooth(&config);

        signal(SIGINT, sig_handler);
        signal(SIGKILL, sig_handler);
        signal(SIGSTOP, sig_handler);
        signal(SIGTERM, sig_handler);

        if (config.use_gps) // Caso colocou o argumento g, e ativou o gps.
        {
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

                int *ptr;
                pthread_join(gps_thread, (void **)&ptr);
                gps_close(&gpsdata);
        }
        else
        {
                while (true)
                {
                        if (kill_program)
                                break;
                        advertise_le();
                }
        }
}