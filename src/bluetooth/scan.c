#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

int main()
{
    int dev_id = hci_devid("hci0");
    if (dev_id < 0)
        dev_id = hci_get_route(NULL);

    int dd = hci_open_dev(dev_id);

    if (dd < 0) {
        perror("Erro ao abrir o socket HCI");
        exit(1);
    }

    // Configura o filtro para eventos LE Meta
    struct hci_filter filter;
    hci_filter_clear(&filter);
    hci_filter_set_ptype(HCI_EVENT_PKT, &filter);
    hci_filter_set_event(EVT_LE_META_EVENT, &filter);
    setsockopt(dd, SOL_HCI, HCI_FILTER, &filter, sizeof(filter));

    // Ativa o modo LE Scan
    hci_le_set_scan_parameters(dd, 0x01, htobs(0x0010), htobs(0x0010), 0x00, 0x00, 1000);
    hci_le_set_scan_enable(dd, 0x01, 0x00, 1000);

    while (1) {
        unsigned char buffer[HCI_MAX_EVENT_SIZE];
        int len = read(dd, buffer, sizeof(buffer));

        if (len >= HCI_EVENT_HDR_SIZE) {
            evt_le_meta_event *meta_event = (evt_le_meta_event *)(buffer + HCI_EVENT_HDR_SIZE + 1);
            if (meta_event->subevent == EVT_LE_ADVERTISING_REPORT) {
                le_advertising_info *info = (le_advertising_info *)(meta_event->data + 1);
                char addr[18];
                ba2str(&(info->bdaddr), addr);
                printf("Dispositivo encontrado: %s\n", addr);
            }
        }
    }

    // Desativa o modo LE Scan
    hci_le_set_scan_enable(dd, 0x00, 0x00, 1000);

    close(dd);

    return 0;
}
