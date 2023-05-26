# Arquivos

*adv.c* funciona com o protocolo Remote ID zerado. Para criar um executável do caminho padrão e executar use:
```bash
sudo apt-get install --reinstall -y bluez libgps-dev
sudo gcc -o advLE ./src/bluetooth/advLE.c $(pkg-config --libs --cflags bluez libgps) -lm
sudo ./advLE
```
Lembre de ter bluez instalado em sua máquina.

Teve erro de pthread? Adicione -pthread ao fim do seu comando de compilação.


*beaconAdv.py* cria um AltBeacon Advertisement. Precisa de remodelar para o RemoteID.

*advertizer.c* envia um advertise de teste, mas não beacon.

*scan.c* é uma tentativa de scan em c para ver os dispositivos bluetooth.

*wifi_sender* ainda nao funciona.
