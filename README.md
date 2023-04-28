# Tunelování datových přenosů přes DNS dotazy, ISA projekt
#### Autor: Veronika Molnárová, xmolna08
##### Dátum: 12.11.2022 
---

## Popis projektu
Projekt pozostáva z vytvorenia serverovej a klientskej aplikácie.
Serverová časť by mala simulovať fungovanie DNS serveru, ktor7 počúva na UDP DNS porte a spacováva prichádzajúce dotazy. Namiesto odpovedania na dané dotazy, sú prichádzajúce dotazy dešifrované a dáta uložené do súboru. V prípade príchodu *truncated* packetu sa server prepína na komunikáciu TCP, kde následne spracováva zvyšné packety od klienta.
Klientská časť aplikácie číta dáta zo zadaného vstupu, kóduje ich a následne posiela dotazy s dátami, cestou k súboru, kde dáta majú byť na serveri uložené a doménou, na ktorú je daný dotaz posielaný, na server.

## Zpôsob spúšťania projektu

K danému projektu je priložený súbor Makefile slúžiaci na preloženie projektu. Projekt sa prekladá pomocou zavolania príkažu *make*, ktorý preloží obe časti aplikácie v jednotlivých priečinkoch *sender* a *receiver*. V prípade zavolania programu s nesprávnou syntaxou je užívateľovi vypísaná nápoveda. Po spustení serveru, server čaká na prichádzajúce dotazy, zatiaľ čo po spustení klienta, sa klient začne dotazovať na server. Napojenie na DNS port u serverskej časti môže vyžadovať prístup superužívateľa.

Príklad prekladu programu:
```sh
$ make
gcc -o receiver/dns_receiver.o -c receiver/dns_receiver.c
gcc -c -o dns_coding.o dns_coding.c
gcc -c -o dns_packet.o dns_packet.c
gcc -o receiver/dns_receiver_events.o -c receiver/dns_receiver_events.c
gcc receiver/dns_receiver.o dns_coding.o dns_packet.o receiver/dns_receiver_events.o -o receiver/dns_receiver
gcc -o sender/dns_sender.o -c sender/dns_sender.c
gcc -o sender/dns_sender_events.o -c sender/dns_sender_events.c
gcc sender/dns_sender.o dns_coding.o dns_packet.o sender/dns_sender_events.o -o sender/dns_sender
```

Príklad spustenia serverskej časti s komunikáciou:

```sh
$ cd receiver
$ ./dns_receiver google.com save_dir
[INIT] 127.0.0.1
[PARS] text.txt 'OVTWQaDINBUGQaDINBUGQLROFYXAaCTUMFVXUZJAORZGKdDJEBVXEYLUEBYGScY'
[PARS] text.txt 'MVWSAdDFNZaGeIDUMVcHIIDMMVRGeIDONdaGKcDBMQQGUZJAOdSWScTEFQGQUbI'
[PARS] text.txt 'NdSCAZDMNBSSAZDBORQSAbTBEBaWIcBMBUFGedDWNdZGSIDTMEQHIYbQFQGQUYQ'
[PARS] text.txt 'OVSGKIDUNcQGMdLOMdXXMYLUEAGQUcDMMU'
[RECV] text.txt     44411 138B from 127.0.0.1
[PARS] text.txt 'MFZWKLROFYXCcDIKORXXIbZANJSSAabBORVWSbTBEBZXAcTBOZQQaCTENZSXGIA'
[PARS] text.txt 'OJQWcbZAONXWaIDCNdWGCIDOMEQGeYTFMRSSAYJANVQWYaJANdZWGaDOOVaGKIA'
[PARS] text.txt 'NBZGCbTPNRVXSIDQOJUWGbbNEBWWeaRAONQWYYLUEBRGebBAOVUGecTLNdbHSIA'
[PARS] text.txt 'MFWGKIDNMFWGeIDPONWGCZDFNZcQaCTLOVRWQYLSNNQSAbLJEA'
[RECV] text.txt     53918 148B from 127.0.0.1
[PARS] text.txt 'OBXXMZLEMFWGCIDOMVRWQIDJMRSWaIDENdWWedRANNSWIIDTMEQHAbDBNZaWUZI'
[PARS] text.txt 'NUQHGdDBPJXXMYLUEBQWYZJANJQSAcbPNUQGCbRAORXSAcDPOZSWIYLMMEQHUZI'
[PARS] text.txt 'EBZWebJAONUSAdDPEBdGCcDMMFaGSbDBEBQSAeTFEBaGeIDOMVUSAZLKEBWWeaQ'
[PARS] text.txt 'MEQGGaDZMJQQaCQ'
[RECV] text.txt     23211 126B from 127.0.0.1
[CMPL] text.txt of 412B
```

Príklad spustenia klientskej časti s komunikáciou:
```sh
$ cd sender
$ ./dns_sender -u 127.0.0.1 google.com save_file.txt send.txt
[INIT] 127.0.0.1
[ENCD] text.txt     27349 'OVTWQaDINBUGQaDINBUGQLROFYXAaCTUMFVXUZJAORZGKdDJEBVXEYLUEBYGScY'
[ENCD] text.txt     27349 'MVWSAdDFNZaGeIDUMVcHIIDMMVRGeIDONdaGKcDBMQQGUZJAOdSWScTEFQGQUbI'
[ENCD] text.txt     27349 'NdSCAZDMNBSSAZDBORQSAbTBEBaWIcBMBUFGedDWNdZGSIDTMEQHIYbQFQGQUYQ'
[ENCD] text.txt     27349 'OVSGKIDUNcQGMdLOMdXXMYLUEAGQUcDMMU'
[SENT] text.txt     27349 138B to 127.0.0.1
[ENCD] text.txt     54252 'MFZWKLROFYXCcDIKORXXIbZANJSSAabBORVWSbTBEBZXAcTBOZQQaCTENZSXGIA'
[ENCD] text.txt     54252 'OJQWcbZAONXWaIDCNdWGCIDOMEQGeYTFMRSSAYJANVQWYaJANdZWGaDOOVaGKIA'
[ENCD] text.txt     54252 'NBZGCbTPNRVXSIDQOJUWGbbNEBWWeaRAONQWYYLUEBRGebBAOVUGecTLNdbHSIA'
[ENCD] text.txt     54252 'MFWGKIDNMFWGeIDPONWGCZDFNZcQaCTLOVRWQYLSNNQSAbLJEA'
[SENT] text.txt     54252 148B to 127.0.0.1
[ENCD] text.txt     26433 'OBXXMZLEMFWGCIDOMVRWQIDJMRSWaIDENdWWedRANNSWIIDTMEQHAbDBNZaWUZI'
[ENCD] text.txt     26433 'NUQHGdDBPJXXMYLUEBQWYZJANJQSAcbPNUQGCbRAORXSAcDPOZSWIYLMMEQHUZI'
[ENCD] text.txt     26433 'EBZWebJAONUSAdDPEBdGCcDMMFaGSbDBEBQSAeTFEBaGeIDOMVUSAZLKEBWWeaQ'
[ENCD] text.txt     26433 'MEQGGaDZMJQQaCQ'
[SENT] text.txt     26433 126B to 127.0.0.1
[CMPL] text.txt of 412B
```


V prípade nastania vnútornej chyby počas behu programu je program ukončený s chybovou hláškou na štandardnom chybovom výstupe.

## Zoznam odovzdaných súborov
- Makefile
- Spoločné zdrojové súbory: dns\_coding.c dns\_coding.h dns\_packet.c dns\_packet.h
- Klientské súbory v priečinku *sender*:
dns\_sender.c dns\_sender.h dns\_sender\_events.c dns\_sender\_events.h
- Serverové súbory v priečinku *receiver*:
dns\_receiver.c dns\_receiver.h dns\_receiver\_events.c dns\_receiver\_events.h
- README.md
- manual.pdf