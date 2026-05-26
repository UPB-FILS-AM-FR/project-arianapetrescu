ALARM SYSTEM

| | |
|-|-|
|`Author` | Petrescu Ariana

## Description
Acest proiect reprezinta un sistem electronic de securitate realizat in timp real cu ajutorul
tehnologiei microcontrolerelor. Scopul sau principal este de a detecta prezenta intrusilor prin
intermediul unui senzor de miscare si de a declansa o alarma sonora si vizuala, care poate fi
dezactivata exclusiv prin introducerea unui cod PIN secret de la o tastatura matriciala.
Procesul tehnologic din spatele sistemului urmeaza un flux logic bine definit:
• Detectia miscare: Senzorul PIR (Passive Infrared) monitorizeaza continuu campul
sau de detectie. Atunci cand detecteaza o variatie de temperatura infrarosie — semn
al prezentei unei persoane — transmite un semnal digital de tip HIGH catre
microcontroler prin pinul D2.
• Procesarea digitala: Placa Arduino Uno preia semnalul de la PIR si aplica logica
conditionala pentru gestionarea starilor sistemului: standby, alarma activa, verificare
PIN, sistem blocat. Microcontrolerul mentine o evidenta a incercarilor gresite si
calculeaza timpii de blocare folosind functia non-blocking millis().
• Afisarea informatiei: Un ecran LCD 16x2, conectat in mod 4-bit direct la Arduino,
afiseaza in timp real starea sistemului: confirmarea activarii, solicitarea PIN-ului,
rezultatul verificarii si countdown-ul de blocare.
• Semnalizarea sonora si vizuala: Un buzzer activ genereaza sunete de alarma
ritmica atunci cand sistemul este in stare de alerta. Doua LED-uri — verde pentru
starea normala si rosu pentru alarma — ofera un feedback vizual instant, vizibil de la
distanta.
• Mecanismul de securitate: Utilizatorul introduce un cod PIN de 4 cifre de la
tastatura 4x4. Codul corect dezactiveaza alarma imediat. Dupa 3 incercari gresite
consecutive, sistemul se blocheaza automat timp de 30 de secunde, prevenind
atacurile prin forta bruta.

## Motivation
Am ales sa realizez acest proiect din dorinta de a intelege cum functioneaza cu adevarat un
sistem de securitate de la zero — nu dintr-o perspectiva de simplu utilizator al unui produs
comercial, ci ca inginer care proiecteaza, conecteaza si programeaza fiecare componenta in
parte.
Sistem de Alarma cu Arduino Uno Proiect Sisteme Embedded
Pagina 2
Sistemele de alarma sunt prezente in viata cotidiana: acasa, la scoala, in magazine. Insa de
cele mai multe ori, mecanismul lor intern ramane invizibil. Acest proiect a reprezentat
oportunitatea de a demistifica aceasta tehnologie si de a reproduce, la scara redusa, acelasi
principiu pe care se bazeaza sisteme valorand mii de euro: un senzor, un procesor si un
mecanism de autentificare.
Pe plan personal, am fost motivat de dorinta de a aplica in practica notiunile teoretice despre
semnale digitale, logica conditionala si sisteme embedded. Exista o satisfactie aparte in
momentul in care PIR-ul detecteaza miscarea, buzzerul porneste, LED-ul rosu se aprinde si
LCD-ul afiseaza „!!! ALARMA !!!” — totul orchestrat de un cod scris linie cu linie.
In plus, proiectul m-a obligat sa rezolv probleme reale de inginerie: gestionarea a 20 de pini
simultan, eliminarea conflictelor intre biblioteci, calibrarea senzorului PIR, conectarea LCDului in mod 4-bit fara modul I2C si implementarea unui timer non-blocking pentru a nu bloca
citirea tastaturii in timp ce buzzerul suna. Fiecare obstacol depasit a consolidat intelegerea
mea asupra modului in care hardware-ul si software-ul interactioneaza intr-un sistem
embedded real.
## Architecture
Arhitectura proiectului defineste structura bloc-in-bloc a sistemului si modul in care
informatia si energia circula intre componente. Aceasta este impartita in trei niveluri
principale: Hardware, Software si Alimentare/Energie.
1. Arhitectura Hardware (Fluxul de Date)
Sistemul este configurat dupa o topologie de tip Liniara, unde datele circula intr-un singur
sens, de la achizitie pana la executie:
• Blocul de Achizitie — Senzorul PIR: Detecteaza variatiile de radiatie infrarosie din
campul sau de vizibilitate (unghi tipic: 120 de grade, raza: 3–7 metri). Semnalul de
iesire este de tip digital (HIGH/LOW) si este transmis catre pinul D2 al Arduino.
• Blocul de Achizitie — Tastatura 4x4: Cei 8 pini ai tastaturii matriciale (4 randuri + 4
coloane) sunt conectati la pinii A0–A3 (randuri) si D3–D6 (coloane). Biblioteca Keypad
scaneaza matricea si detecteaza apasarile de taste.
• Blocul de Procesare — Arduino Uno: Microcontrolerul ATmega328P centralizeaza
toate semnalele, aplica logica de control si genereaza comenzile pentru dispozitivele
de iesire. Ruleaza la 16 MHz si administreaza 20 de pini simultan.
• Blocul de Executie — Iesiri: Prin pinii D7–D12 controleaza LCD-ul (mod 4-bit). Prin
A5 activeaza buzzerul. Prin D13 si A4 comanda LED-urile rosu si verde.
2. Arhitectura de Alimentare (Managementul Energiei)
Pentru a asigura stabilitatea si a preveni supraincarcarea microcontrolerului, alimentarea
este distribuita prin doua magistrale conectate printr-o masa comuna:
Sistem de Alarma cu Arduino Uno Proiect Sisteme Embedded
Pagina 3
• Sursa principala: Arduino Uno este alimentat prin cablul USB de la calculator (sau
sursa externa de 5V). Pinii 5V si GND ai placii sunt conectati la magistralele
breadboard-ului.
• Magistrala de 5V (linia rosie): Distribuie tensiunea catre PIR (VCC), LCD (VDD,
BLA prin 220Ω) si LED-uri (prin rezistente de 220Ω fiecare).
• Magistrala de GND (linia albastra): Toate bornele de minus sunt conectate
impreuna: Arduino, PIR, LCD (GND, RW, V0), Buzzer, LED-uri. Fara aceasta masa
comuna, semnalele nu ar avea un punct de referinta comun.
• Protectia LED-urilor: Fiecare LED are o rezistenta de 220Ω in serie, care limiteaza
curentul la circa 14mA, sub limita de 40mA a pinilor Arduino.
3. Arhitectura Software (Logica Codului)
Codul rulat pe Arduino urmeaza o arhitectura de tip Masina de Stari (State Machine), cu
urmatoarele stari posibile:
[ STANDBY ] → [ ALARMA ACTIVA ] → [ VERIFICARE PIN ] → [ DEZACTIVAT /
BLOCAT ]
Starea STANDBY
• LED Verde aprins, LCD afiseaza „Sistem ACTIV”
• PIR-ul este citit continuu la fiecare ciclu loop()
• Daca PIR = HIGH → tranzitie imediata la ALARMA ACTIVA
Starea ALARMA ACTIVA
• LED Rosu aprins, buzzer ritmic pornit, LCD afiseaza „!!! ALARMA !!!”
• Tastatura este scanata continuu pentru introducerea PIN-ului
• # = sterge ultima cifra, * = reseteaza codul complet
• La 4 cifre introduse → tranzitie la VERIFICARE PIN
Starea VERIFICARE PIN
• Codul introdus este comparat cu constanta COD_SECRET
• Cod corect: 3 bipuri scurte, LED verde, sistem revine la STANDBY
• Cod gresit: 2 bipuri lungi, incrementare contor greseli, sistem revine la ALARMA
• La 3 greseli consecutive → tranzitie la BLOCAT
Starea BLOCAT
• Sistem blocat 30 de secunde, LCD afiseaza countdown-ul in timp real
• Implementat cu millis() (non-blocking), astfel incat buzzerul continua sa sune si
LCD-ul sa se actualizeze)
• La expirare → resetare completa, tranzitie la STANDBY
### Block diagram

<img width="1667" height="1124" alt="image" src="https://github.com/user-attachments/assets/16c8f069-3bd3-43c3-a731-36e11bd14157" />

### Schematic

![Schematic](schematics/kicad_schematic.png)

### Components


<!-- This is just an example, fill in with your actual components -->

| Device | Usage | Price |
|--------|--------|-------|
| Activ Buzzer | Buzzer | [1.5 RON](https://www.optimusdigital.ro/ro/audio-buzzere/635-buzzer-activ-de-3-v.html?search_query=buzzer&results=61) |
| Push Button | Button | [1 RON](https://www.optimusdigital.ro/ro/butoane-i-comutatoare/1119-buton-6x6x6.html?search_query=buton&results=222) |
| Jumper Wires | Connecting components | [7 RON](https://www.optimusdigital.ro/ro/fire-fire-mufate/884-set-fire-tata-tata-40p-10-cm.html?search_query=set+fire&results=110) |
| Breadboard | Project board | [10 RON](https://www.optimusdigital.ro/ro/prototipare-breadboard-uri/8-breadboard-830-points.html?search_query=breadboard&results=145) |

### Libraries

<!-- This is just an example, fill in the table with your actual components -->

| Library | Description | Usage |
|---------|-------------|-------|
| [lib-name1](link-to-lib) | official description of the lib | Used for accesing the peripherals of the microcontroller  |
| [lib-name2](link-to-lib) | official description of the lib | Used for accesing the peripherals of the microcontroller  |

## Log

<!-- write every week your progress here -->

### Week 6 - 12 May

### Week 7 - 19 May

### Week 20 - 26 May


## Reference links

<!-- Fill in with appropriate links and link titles -->

[Tutorial 1](https://www.youtube.com/watch?v=wdgULBpRoXk&t=1s&ab_channel=BenEater)

[Article 1](https://www.explainthatstuff.com/induction-motors.html)

[Link title](https://projecthub.arduino.cc/)
