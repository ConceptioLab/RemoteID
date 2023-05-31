#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

static void hci_le_reset_advertising_scan(int dd)
{
    uint8_t ogf = OGF_HOST_CTL;
    uint16_t ocf = OCF_RESET;
    uint8_t buf[] = {};

    if (hci_send_cmd(dd, ogf, ocf, sizeof(buf), buf) < 0)
    {
        perror("Failed to send HCI command");
        exit(EXIT_FAILURE);
    }
}

int main()
{
    int dev_id = hci_get_route(NULL);
    int dd = hci_open_dev(dev_id);

    if (dd < 0)
    {
        perror("Erro ao abrir o socket HCI");
        exit(EXIT_FAILURE);
    }
    hci_le_reset_advertising_scan(dd);

    struct hci_filter old_filter;
    socklen_t old_filter_len = sizeof(old_filter);
    if (getsockopt(dd, SOL_HCI, HCI_FILTER, &old_filter, &old_filter_len) < 0)
    {
        perror("Failed to get socket options");
        exit(EXIT_FAILURE);
    }

    struct hci_filter new_filter;
    hci_filter_clear(&new_filter);
    hci_filter_set_ptype(HCI_EVENT_PKT, &new_filter);
    hci_filter_set_event(EVT_LE_META_EVENT, &new_filter);
    if (setsockopt(dd, SOL_HCI, HCI_FILTER, &new_filter, sizeof(new_filter)) < 0)
    {
        perror("Failed to set socket options");
        exit(EXIT_FAILURE);
    }

    hci_le_set_scan_parameters(dd, 0x00, htobs(0x0010), htobs(0x0010), 0x00, 0x00, 1000);
    le_set_scan_enable(dd, 0x01, 0x00, 1000);

    printf("Start discovery initiated.\n");

    // Agora você pode esperar pelos eventos de descoberta e tratá-los aqui.
    while (1)
    {
        unsigned char buffer[HCI_MAX_EVENT_SIZE];
        ssize_t len = read(dd, buffer, sizeof(buffer));

        if (len < 0)
        {
            perror("Failed to read HCI event");
            break;
        }
        

        evt_le_meta_event *meta_event = (evt_le_meta_event *)(buffer + HCI_EVENT_HDR_SIZE + 1);

        le_advertising_info *info = (le_advertising_info *)(meta_event->data + 1);
        char addr[18];
        ba2str(&(info->bdaddr), addr);
        printf("Dispositivo encontrado: %s\n", addr);
    }

    hci_le_set_scan_enable(dd, 0x00, 0x00, 10000); // Desabilitar a varredura após a descoberta
    printf("Scan finalizado\n");

    if (setsockopt(dd, SOL_HCI, HCI_FILTER, &old_filter, sizeof(old_filter)) < 0)
    {
        perror("Failed to restore socket options");
    }

    hci_close_dev(dd);

    return 0;
}
