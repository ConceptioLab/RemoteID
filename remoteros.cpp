#include <errno.h>
#include <thread>
#include "../include/gpsmod.h"

#include "advle.h"
#include "scan.h"

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.h"

#include <sstream>

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
                case 'l':
                        config->use_btl = true;
                        break;
                case 'g':
                        config->use_gps = true;
                        break;
                case 's':
                        config->use_scan = true;
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

        // Setting bluetooth for advertising
        const int device = open_hci_device();
        init_bluetooth();

	rclcpp::init(argc, argv, "remoteID");
        rclcpp::NodeHandle n;
        rclcpp::Publisher chatter_pub = n.advertise<std_msgs::String>("conceptio/unit/air/simulation/drone/remoteid", 1000);
        rclcpp::Rate loop_rate(10);

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
		void * data = (void *)&args;

		int ret = pthread_create(&gps_thread, NULL, &gps_thread_function, data);
                if (ret != 0)
                {
                        fprintf(stderr, "Falha ao criar a pthread.\n");
                        return 1;
                }

                while (true)
                {
                        if (kill_program)
                                break;
                        if (config.use_btl)
                        {
                                advertise_le();
                        }
                        if (config.use_scan)
                        {
                                scan_le();
                        }
                }
                // Fecha a conexão com o GPS
                gps_stream(args.gpsdata, WATCH_DISABLE, NULL);
                gps_close(args.gpsdata);

                pthread_exit(0);
        }
        else
        {
                fill_example_gps_data(&uasData);

                printf("GPS: DESLIGADO\n");
                while (true)
                {
                        if (kill_program)
                                break;
                        if (config.use_btl)
                        {
                                advertise_le();
                        }
                        if (config.use_scan)
                        {
                                scan_le();
                        }
			if (rclcpp::ok())
                        {
                                std_msgs::String msg;
                                std::stringstream ss;
                                ss << &uasData.SelfID;
                                msg.data = ss.str();
                                rclcpp_INFO("%s", msg.data.c_str());

                                chatter_pub.publish(msg);
                        }
                }
        }

        cleanup(EXIT_SUCCESS);
}

