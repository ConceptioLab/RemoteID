# RemoteID
O intuito do Remote ID é prover a identificação de um drone para qualquer parte interessada.
Tudo é baseado no repositório https://github.com/opendroneid/opendroneid-core-c/ , que tem como base o protocolo criado pela American Society for Testing and Materials International, F3411-22a.

Dentro da pasta src você encontrará instruções para rodar exemplos de Remote ID usando máquinas linux.

O foco desse projeto é além de prover essa identificação, utiliza-la para que aeronaves nas proximidades consigam se identificar sem ajuda externa.

# Entendendo esse repositório

Os tipos de comunicação irão ficar separados em **2 pastas** dentro de *src*, bluetooth para sinais bluetooth, e wifi para sinais wi-fi.

# Executando Remote ID linux - Advertise e Scan

```bash
sudo apt-get install --reinstall -y bluez libgps-dev libconfig-dev
sudo gcc ./src/bluetooth/remote.c ./src/bluetooth/advle.c ./src/bluetooth/scan.c $(pkg-config --libs --cflags bluez libgps) -lm -pthread -o remote
```

Executar o arquivo

```bash
sudo ./remote "arg"
```

Substitua "arg" por:

l para enviar os dados em advertise.

g para ativar o gps e enviar os dados do gps.

s para ativar o scan e buscar sinais remote ID próximos e salvar no json apropriado.

Exemplo: 

```bash
sudo ./remote l
sudo ./remote l g s
```
