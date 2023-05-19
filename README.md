# RemoteID
O intuito do Remote ID é prover a identificação de um drone para qualquer parte interessada.

O foco desse projeto é além de prover essa identificação, utiliza-la para que aeronaves nas proximidades consigam se identificar sem ajuda externa.

# Entendendo esse repositório
Atualmente, a pasta server está ali para testar conexões bluetooth diretas com os diferentes tipos de comunicação.

Os tipos de comunicação irão ficar separados em **2 pastas** dentro de *src*, bluetooth para sinais bluetooth, e wifi para sinais wi-fi.

## Arquivos

beaconAdv.py cria um AltBeacon Advertisement. Precisa de remodelar para o RemoteID.

advertizer.c envia um advertise de teste, mas não beacon.

scan.c é uma tentativa de scan em c para ver os dispositivos bluetooth.

wifi_sender ainda nao funciona.

remote.c é uma cópia de advertizer.c, e será usado para testes com modificações de advertizer.c



Tudo do opendroneid está para implementação futura.