#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

int main()
{
    int dev_id = hci_get_route(NULL);
    int sock = hci_open_dev(dev_id);
    if (sock < 0)
    {
        perror("Erro ao habilitar adaptador HCI");
        return -1;
    }

    // Definir os parâmetros de advertising
    struct hci_request request;
    le_set_advertising_parameters_cp adv_params{};
    memset(&request, 0, sizeof(request));
    request.ogf = OGF_LE_CTL;
    request.ocf = OCF_LE_SET_ADVERTISING_PARAMETERS;
    request.cparam = &adv_params;
    request.clen = LE_SET_ADVERTISING_PARAMETERS_CP_SIZE;
    request.rparam = NULL;
    request.rlen = 0;

    if (hci_send_req(sock, &request, 1000) < 0)
    {
        perror("Erro ao configurar parâmetros de advertising");
        hci_close_dev(sock);
        return -1;
    }

    // Definir os dados de advertising
    struct hci_request adv_data_request;
    le_set_advertising_data_cp adv_data{};
    memset(&adv_data_request, 0, sizeof(adv_data_request));
    adv_data_request.ogf = OGF_LE_CTL;
    adv_data_request.ocf = OCF_LE_SET_ADVERTISING_DATA;
    adv_data_request.cparam = &adv_data;
    adv_data_request.clen = LE_SET_ADVERTISING_DATA_CP_SIZE;
    adv_data_request.rparam = NULL;
    adv_data_request.rlen = 0;

    uint8_t data[] = {
        0x02, 0x01, 0x06,  // Flags: General Discoverable Mode, BR/EDR Not Supported
        0x09, 0x09, 'H', 'e', 'l', 'l', 'o'  // Complete Local Name: "Hello"
    };
    uint8_t dataLength = sizeof(data);
    memcpy(adv_data.data, data, dataLength);

    if (hci_send_req(sock, &adv_data_request, 1000) < 0)
    {
        perror("Erro ao configurar dados de advertising");
        hci_close_dev(sock);
        return -1;
    }

    // Habilitar o advertising
    uint8_t enable = 0x01;
    if (hci_le_set_advertise_enable(sock, enable, 1000) < 0)
    {
        perror("Erro ao habilitar o advertising");
        hci_close_dev(sock);
        return -1;
    }

    printf("Advertising habilitado. Pressione Ctrl+C para encerrar.\n");

    // Aguardar indefinidamente
    while (1)
    {
        usleep(1000000);  // Aguardar 1 segundo
    }

    // Desabilitar o advertising antes de fechar o socket Bluetooth
    enable = 0x00;
    hci_le_set_advertise_enable(sock, enable, 1000);

    // Fechar o socket Bluetooth
    hci_close_dev(sock);

    return 0;
}
