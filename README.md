# RemoteID

Default raspberry wifi: concrpi
senha padrão.

O intuito do Remote ID é prover a identificação de um drone para qualquer parte interessada.
Tudo é baseado no repositório https://github.com/opendroneid/opendroneid-core-c/ , que tem como base o protocolo criado pela American Society for Testing and Materials International, F3411-22a.

Dentro da pasta src você encontrará instruções para rodar exemplos de Remote ID usando máquinas linux.

O foco desse projeto é além de prover essa identificação, utiliza-la para que aeronaves nas proximidades consigam se identificar sem ajuda externa.

# Entendendo esse repositório

Os tipos de comunicação irão ficar separados em **2 pastas** dentro de *src*, bluetooth para sinais bluetooth, e wifi para sinais wi-fi.

# Executando Remote ID linux - Advertise e Scan

Lembre-se de entrar na pasta do RemoteID, e executar ao menos o install caso queira usar o cmake.

```bash
sudo apt-get install --reinstall -y bluez libgps-dev libconfig-dev libbluetooth-dev gpsd
sudo gcc ./src/bluetooth/remote.c ./src/bluetooth/advle.c ./src/bluetooth/scan.c $(pkg-config --libs --cflags bluez libgps libconfig) -lm -pthread -o remote
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

GPS não funcionando? Verifique se as conexões estão corretas e se o serial está sendo acessado. (Caso não, ative pelo raspi-config)

## Docker

No docker, lembre de iniciar o container com --network host.

EXEMPLO: 

```bash 
sudo docker run -it --network host -v /dev:/dev -v /opt/RemoteID:/app -p 8000-8050:8000-8050 sua/imagem:docker
```

## CMakeList

Lembre-se de entrar na pasta do RemoteID, executar os comandos do linux e depois caso queira, usar o cmake no futuro.
Criar uma pasta chamada "build", entrar nela e rodar os comandos:

```bash
cmake ..
make
```
Caso queira rodar sem o ros, modificar CMakeList.txt para que remoteros.cpp -> remote.cpp, remover: ```find_package(rclcpp REQUIRED), find_package(rclcpp_components REQUIRED), ${ROS_INCLUDE_DIR}```.

De preferência, mude a linha: ```add_executable(remoteros ${SOURCES})``` -> ```add_executable(remote ${SOURCES})```
