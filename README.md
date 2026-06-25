# IoT-Laite

Tämä laite ilmoittaa seuraavan nopeusvalvontakameran sijainnin.

Repository sisältää nyt pienen FreeRTOS-tyylisen sovellusrungon laitteelle, jonka
komponentit ovat:

- MCU: nRF52840
- GPS: u-blox MAX-M10S
- Storage: SPI Flash 8–16 MB (nykyinen profiili 16 MB)
- Audio: MAX98357A + pieni kaiutin
- Akku: 1000 mAh LiPo

## Sovellusrakenne

Sovellus mallintaa seuraavat tehtävät:

- `storage_task`: alustaa SPI Flash -tallennuksen ja lataa nopeuskameratietokannan
- `gps_task`: vastaanottaa GPS-paikannuksen MAX-M10S-moduulilta
- `navigation_task`: etsii lähimmän seuraavan kameran
- `audio_task`: lähettää puheilmoituksen MAX98357A-vahvistimelle
- `battery_task`: raportoi akun kapasiteetin ja karkean käyttöaika-arvion

Koska repositoryssa ei ollut aiempaa firmwarea tai valmistajakohtaisia SDK-riippuvuuksia,
toteutus on tehty hostilla käännettävänä FreeRTOS-runkona. Mukana oleva
`freertos_host_shim` mahdollistaa sovellusvirran validoinnin ilman varsinaista
FreeRTOS-porttia. Oikealla laitteella shim korvataan FreeRTOS-kernelillä ja
nRF52840-ajureilla.

## Käännös

```sh
cmake -S . -B build
cmake --build build
./build/iot_laite
```
