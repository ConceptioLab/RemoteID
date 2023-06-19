# Arquivos

*adv.c* funciona com o protocolo Remote ID zerado. Para criar um executável do caminho padrão e executar use:

*beaconAdv.py* cria um AltBeacon Advertisement. Precisa de remodelar para o RemoteID.

*advertizer.c* envia um advertise de teste, mas não beacon.

*scan.c* é uma tentativa de scan em c para ver os dispositivos bluetooth.

*wifi_sender* ainda nao funciona.

# Executando Remote ID linux - Advertise e Scan

```bash
sudo apt-get install --reinstall -y bluez libgps-dev
sudo gcc ./src/bluetooth/remote.c ./src/bluetooth/advle.c ./src/bluetooth/scan.c $(pkg-config --libs --cflags bluez libgps) -lm -pthread -o remote
```

Executar normalmente

```bash
sudo ./remote arg
```

Substitua "arg" por:

l para executar normalmente.

g para ativar o gps.

Exemplo: 

```bash
sudo ./remote l g
```
