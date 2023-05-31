#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#define BUFFER_SIZE 1024

int main() {
    // Abrir o adaptador Bluetooth
    int adapter_id = hci_get_route(NULL);
    int socket = hci_open_dev(adapter_id);
    if (socket < 0) {
        perror("Erro ao abrir o adaptador Bluetooth");
        exit(1);
    }

    // Configurar o filtro para capturar todos os pacotes
    struct hci_filter filter;
    hci_filter_clear(&filter);
    hci_filter_set_ptype(HCI_EVENT_PKT, &filter);
    hci_filter_set_event(EVT_LE_META_EVENT, &filter);
    if (setsockopt(socket, SOL_HCI, HCI_FILTER, &filter, sizeof(filter)) < 0) {
        perror("Erro ao configurar o filtro");
        close(socket);
        exit(1);
    }

    // Iniciar a captura de pacotes
    unsigned char buffer[BUFFER_SIZE];
    while (1) {
        int length = read(socket, buffer, sizeof(buffer));
        if (length < 0) {
            perror("Erro ao ler o pacote");
            close(socket);
            exit(1);
        }

        // Verificar se é um pacote LE Meta Event
        if (buffer[0] == HCI_EVENT_PKT && buffer[1] == EVT_LE_META_EVENT) {
            int subevent_code = buffer[3];
            if (subevent_code == EVT_LE_ADVERTISING_REPORT) {
                // Obter os dados do pacote de publicidade
                int num_reports = buffer[4];
                int offset = 0;
                for (int i = 0; i < num_reports; i++) {
                    offset = 9 + (i * 13);
                    uint8_t addr_type = buffer[offset];
                    bdaddr_t addr;
                    memcpy(&addr, &buffer[offset + 1], sizeof(bdaddr_t));
                    int8_t rssi = buffer[offset + 7];
                    int data_length = buffer[offset + 8];
                    uint8_t *data = &buffer[offset + 9];

                    // Exibir informações do dispositivo
                    char addr_str[18];
                    ba2str(&addr, addr_str);
                    printf("Dispositivo: %s, RSSI: %d dBm\n", addr_str, rssi);
                    printf("Dados do pacote: ");
                    for (int j = 0; j < data_length; j++) {
                        printf("%02x ", data[j]);
                    }
                    printf("\n\n");
                }
            }
        }
    }

    // Fechar o soquete e liberar recursos
    close(socket);
    return 0;
}
