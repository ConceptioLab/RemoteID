# Arquivos

*adv.c* funciona com o protocolo Remote ID zerado. Para criar um executável do caminho padrão e executar use:

*beaconAdv.py* cria um AltBeacon Advertisement. Precisa de remodelar para o RemoteID.

*advertizer.c* envia um advertise de teste, mas não beacon.

*scan.c* é uma tentativa de scan em c para ver os dispositivos bluetooth.

*wifi_sender* ainda nao funciona.

# Executando Remote ID linux

```bash
sudo apt-get install --reinstall -y bluez libgps-dev
sudo gcc -o advLE ./src/bluetooth/advLE.c $(pkg-config --libs --cflags bluez libgps) -lm
```
Teve erro de pthread? Adicione -pthread ao fim do seu comando de compilação.

Executar normalmente

```bash
sudo ./advLE arg
```

Substitua "arg" por:

l para executar normalmente.

g para ativar o gps.

Exemplo: 

```bash
sudo ./advLE l g
```