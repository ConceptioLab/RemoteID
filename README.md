# RemoteID
O intuito do Remote ID é prover a identificação de um drone para qualquer parte interessada.
Tudo é baseado no repositório https://github.com/opendroneid/opendroneid-core-c/ , que tem como base o protocolo criado pela American Society for Testing and Materials International, F3411-22a.

Dentro da pasta src você encontrará instruções para rodar exemplos de Remote ID usando máquinas linux.

O foco desse projeto é além de prover essa identificação, utiliza-la para que aeronaves nas proximidades consigam se identificar sem ajuda externa.

# Entendendo esse repositório
Atualmente, a pasta server está ali para testar conexões bluetooth diretas com os diferentes tipos de comunicação.

Os tipos de comunicação irão ficar separados em **2 pastas** dentro de *src*, bluetooth para sinais bluetooth, e wifi para sinais wi-fi.
