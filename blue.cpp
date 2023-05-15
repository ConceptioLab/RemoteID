#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <unistd.h>

int main() {
    // Endereço MAC do dispositivo remoto
    const char* remoteAddress = "48:2c:a0:f9:71:c6";

    // Criação do socket Bluetooth
    int bSocket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (bSocket < 0) {
        perror("Failed to create Bluetooth socket");
        return -1;
    }

    // Preenche a estrutura de endereço Bluetooth
    struct sockaddr_rc addr = { 0 };
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t)1;  // Canal RFCOMM para comunicação Bluetooth

    // Converte o endereço MAC para uma estrutura de endereço Bluetooth
    str2ba(remoteAddress, &addr.rc_bdaddr);

    // Conecta ao dispositivo remoto
    if (connect(bSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Failed to connect to remote device");
        close(bSocket);
        return -1;
    }

    // Dados a serem enviados
    const char* data = "Hello, Bluetooth!";

    // Envia os dados pelo socket Bluetooth
    if (write(bSocket, data, strlen(data)) < 0) {
        perror("Failed to send data via Bluetooth");
    }

    // Fecha o socket Bluetooth
    close(bSocket);

    return 0;
}
