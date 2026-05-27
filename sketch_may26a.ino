/*
 * ============================================================
 *   SISTEM DE ALARMA CU ARDUINO UNO — Versiunea finala
 * ============================================================
 *  D2  ← PIR OUT
 *  D3  ← Keypad Col 1
 *  D4  ← Keypad Col 2
 *  D5  ← Keypad Col 3
 *  D6  ← Keypad Col 4
 *  D7  → LCD RS
 *  D8  → LCD E
 *  D9  → LCD D4
 *  D10 → LCD D5
 *  D11 → LCD D6
 *  D12 → LCD D7
 *  D13 → LED Rosu (+ 220 ohm)
 *  A0  ← Keypad Rand 1
 *  A1  ← Keypad Rand 2
 *  A2  ← Keypad Rand 3
 *  A3  ← Keypad Rand 4
 *  A4  → LED Verde (+ 220 ohm)
 *  A5  → Buzzer (+)
 * ============================================================
 */

#include <Keypad.h>
#include <LiquidCrystal.h> //Includem bibliotecile necesare — una pentru keypad, una pentru ecranul LCD.

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(7, 8, 9, 10, 11, 12); //Creăm obiectul LCD și îi spunem la ce pini e conectat

// Pini
const int PIN_PIR     = 2;
const int PIN_LED_ROZ = 13;
const int PIN_LED_VRD = A4;
const int PIN_BUZZER  = A5; //Definim constantele pentru pini

// Keypad
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
}; //Matricea care definește ce caracter corespunde fiecărei taste de pe keypad.

byte rowPins[ROWS] = {A0, A1, A2, A3};
byte colPins[COLS] = {3, 4, 5, 6}; //Pinii la care sunt conectate rândurile și coloanele keypad-ului.

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
//Se creează obiectul keypad folosind matricea de taste și pinii definiți mai sus.

const String COD_SECRET = "1234";

// Variabile
bool alarmaActiva      = false;
String codIntroduced   = "";
int incercariGresite   = 0;
const int MAX_INCERCARI = 3;
bool sistemBlocat      = false;
unsigned long timpBlocare = 0;
const long DURATA_BLOCARE = 30000;

// Variabile PIR
// PIR-ul e citit o data la 20 secunde si trebuie sa fie HIGH timp de 10 secunde ca sa declanseze alarma
unsigned long ultimaCitirePIR = 0;
unsigned long inceputHIGH     = 0;
bool pirEraHigh               = false;
const long INTERVAL_CITIRE    = 20000; // 20 secunde intre citiri
const long DURATA_HIGH_NEEDED = 10000; // 10 secunde continuu HIGH

// ============================================================
void setup() { // rulează o singură dată la pornire.
  Serial.begin(9600); //Pornim comunicarea cu calculatorul prin USB, la viteza 9600 baud — ca să vedem mesaje în Serial Monitor.

  pinMode(PIN_PIR,     INPUT);
  pinMode(PIN_BUZZER,  OUTPUT);
  pinMode(PIN_LED_ROZ, OUTPUT);
  pinMode(PIN_LED_VRD, OUTPUT); //Setează direcția pinilor — PIR e intrare (citim de la el), restul sunt ieșiri (trimitem semnal la ei).

  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP); //Activăm rezistențele interne pull-up pentru rândurile keypad-ului — fără ele pinii "plutesc" și generează apăsări fantomă.

  digitalWrite(PIN_BUZZER,  LOW);
  digitalWrite(PIN_LED_ROZ, LOW);
  digitalWrite(PIN_LED_VRD, LOW); //La pornire, buzzerul și ambele LED-uri sunt stinse.

  lcd.begin(16, 2);

  Serial.println("=================================");
  Serial.println("  SISTEM ALARMA - Se porneste...");
  Serial.println("  Calibrare PIR: asteapta 30s");
  Serial.println("=================================");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrare PIR...");
  lcd.setCursor(0, 1);
  lcd.print("Asteapta 30s...");
  //Senzorul PIR are nevoie de timp să se stabilizeze la pornire.

  delay(30000);

  Serial.println(">> Calibrare terminata!");
  Serial.println(">> Sistem ACTIV - monitorizez...");
  Serial.println("---------------------------------");

  ultimaCitirePIR = millis(); // pornim timerul PIR dupa calibrare

  afiseazaStareNormala();
  digitalWrite(PIN_LED_VRD, HIGH); //După calibrare, afișăm ecranul normal și aprindem LED-ul verde — sistemul e activ și monitorizează.
}

// ============================================================
void loop() {

  // ── Sistem blocat ──
  if (sistemBlocat) {
    unsigned long acum = millis();
    if (acum - timpBlocare >= DURATA_BLOCARE) { //Dacă sistemul e blocat, verificăm dacă au trecut cele 30 de secunde.
      sistemBlocat     = false;
      incercariGresite = 0;
      alarmaActiva     = false;
      codIntroduced    = "";
      digitalWrite(PIN_BUZZER,  LOW);
      digitalWrite(PIN_LED_ROZ, LOW);
      digitalWrite(PIN_LED_VRD, HIGH);
      afiseazaStareNormala(); //Au trecut 30 de secunde — resetăm totul și revenim la starea normală de monitorizare.
      Serial.println(">> Sistem DEBLOCAT. Revin la monitorizare.");
      Serial.println("---------------------------------");
    } else {
      long secRamase = (DURATA_BLOCARE - (acum - timpBlocare)) / 1000;
      lcd.setCursor(0, 1);
      lcd.print("Blocat: ");
      lcd.print(secRamase);
      lcd.print("s   ");
      buzzerRitmic();
    }
    return; //Nu au trecut încă 30 de secunde — afișăm countdown-ul pe LCD și continuăm să sunăm buzzerul. return oprește restul funcției loop.
  }

  // ── Citim PIR o data la 20 secunde, confirmare 10 secunde ──
  if (!alarmaActiva) {
    unsigned long acum = millis();

    if (acum - ultimaCitirePIR >= INTERVAL_CITIRE) {
      // A trecut fereastra de 20 secunde, citim PIR
      bool pirCurent = (digitalRead(PIN_PIR) == HIGH);

      if (pirCurent) {
        if (!pirEraHigh) {
          // PIR tocmai a devenit HIGH — incepem sa masuram 10 secunde
          pirEraHigh   = true;
          inceputHIGH  = acum;
          Serial.println(">> PIR: MISCARE DETECTATA - confirmare 10s...");
        } else {
          // PIR e inca HIGH — verificam daca au trecut 10 secunde
          if (acum - inceputHIGH >= DURATA_HIGH_NEEDED) {
            Serial.println(">> PIR: MISCARE CONFIRMATA 10s - declansez alarma!");
            pirEraHigh      = false;
            ultimaCitirePIR = acum; // resetam timerul
            declansezaAlarma();
          }
        }
      } else {
        // PIR e LOW — resetam tot
        if (pirEraHigh) {
          Serial.println(">> PIR: LOW inainte de 10s - ignorat.");
        }
        pirEraHigh      = false;
        ultimaCitirePIR = acum; // resetam timerul pentru urmatoarea fereastra
        Serial.println(">> PIR: Liniste...");
      }
    }
  }

  // ── Alarma activa → citim keypad ──
  if (alarmaActiva) {
    buzzerRitmic();

    char tasta = keypad.getKey(); //Cât timp alarma e activă, buzzerul sună ritmic și citim dacă s-a apăsat o tastă.
    if (tasta) {
      Serial.print(">> Tasta apasata: ");
      Serial.println(tasta);

      if (tasta == '#') {
        if (codIntroduced.length() > 0) {
          codIntroduced.remove(codIntroduced.length() - 1);
          //Tasta # funcționează ca backspace — șterge ultima cifră introdusă.
          afiseazaCod();
          Serial.print(">> Cod curent: ");
          Serial.println(codIntroduced);
        }
      } else if (tasta == '*') {
        codIntroduced = "";
        afiseazaCod();
        Serial.println(">> Cod sters.");
        //Tasta * șterge tot codul introdus, de la capăt.
      } else if (tasta >= '0' && tasta <= '9') {
        if (codIntroduced.length() < 4) {
          codIntroduced += tasta;
          //Dacă s-a apăsat o cifră și codul nu are încă 4 caractere, adăugăm cifra.
          afiseazaCod();
          Serial.print(">> Cod curent: ");
          for (int i = 0; i < (int)codIntroduced.length(); i++) Serial.print("*");
          Serial.println();
        }
        if (codIntroduced.length() == 4) {
          delay(300);
          verificaCod();
          //Când s-au introdus exact 4 cifre, așteptăm 300ms și verificăm codul.
        }
      }
    }
  }
}

// ============================================================
void declansezaAlarma() {
  alarmaActiva  = true;
  codIntroduced = "";
  digitalWrite(PIN_LED_VRD, LOW);
  digitalWrite(PIN_LED_ROZ, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("!!! ALARMA !!!");
  lcd.setCursor(0, 1);
  lcd.print("PIN: ");
  Serial.println("=================================");
  Serial.println("  !!! ALARMA DECLANSATA !!!      ");
  Serial.println("  Introdu PIN-ul de 4 cifre:     ");
  Serial.println("  (* = sterge, # = backspace)    ");
  Serial.println("=================================");
} //Când se declanșează alarma: setăm flag-ul, stingem LED verde, aprindem LED roșu, afișăm mesaj pe LCD.

void verificaCod() {
  if (codIntroduced == COD_SECRET) {
    codCorect();
  } else {
    codGresit();
  }
} //Comparăm codul introdus cu cel secret și mergem la funcția corespunzătoare.

void codCorect() {
  Serial.println("=================================");
  Serial.println("  >> COD CORECT! Alarma oprita.");
  Serial.println("=================================");

  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_ROZ, LOW);

  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_BUZZER, LOW);
    delay(100);
  } //Cod corect — oprim alarma, stingem LED roșu, buzzerul face 3 bip-uri scurte de confirmare.

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Cod CORECT!  ");
  lcd.setCursor(0, 1);
  lcd.print(" Alarma oprita ");
  delay(2000);

  alarmaActiva     = false;
  codIntroduced    = "";
  incercariGresite = 0;
  pirEraHigh       = false;
  ultimaCitirePIR  = millis(); // resetam timerul PIR
  digitalWrite(PIN_LED_VRD, HIGH);
  afiseazaStareNormala();
  Serial.println(">> Sistem ACTIV - monitorizez...");
  Serial.println("---------------------------------");
} //Resetăm toate variabilele, aprindem LED verde și revenim la ecranul normal.

void codGresit() {
  incercariGresite++;
  Serial.print(">> COD GRESIT! Incercarea ");
  Serial.print(incercariGresite);
  Serial.print(" din ");
  Serial.println(MAX_INCERCARI);

  digitalWrite(PIN_BUZZER, LOW);
  delay(150);
  for (int i = 0; i < 2; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(300);
    digitalWrite(PIN_BUZZER, LOW);
    delay(150);
  } //Cod greșit — incrementăm contorul de greșeli, buzzerul face 2 bip-uri lungi de avertizare.

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Cod GRESIT!  ");
  lcd.setCursor(0, 1);
  lcd.print("Incercare ");
  lcd.print(incercariGresite);
  lcd.print("/");
  lcd.print(MAX_INCERCARI);
  delay(1500);

  codIntroduced = "";

  if (incercariGresite >= MAX_INCERCARI) {
    blocareSistem();
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("!!! ALARMA !!!");
    lcd.setCursor(0, 1);
    lcd.print("PIN: ");
    Serial.println(">> Reintrodu PIN-ul:");
  }
} //Dacă s-au atins 3 greșeli, blocăm sistemul.

void blocareSistem() {
  sistemBlocat = true;
  timpBlocare  = millis();
  Serial.println("=================================");
  Serial.println("  SISTEM BLOCAT 30 secunde!      ");
  Serial.println("  Prea multe incercari gresite.  ");
  Serial.println("=================================");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SISTEM BLOCAT!");
  lcd.setCursor(0, 1);
  lcd.print("Asteptati 30s...");
} //Setăm sistemul ca blocat, salvăm momentul blocării și afișăm mesaj pe LCD.

void buzzerRitmic() {
  if ((millis() % 1000) < 500) {
    digitalWrite(PIN_BUZZER, HIGH);
  } else {
    digitalWrite(PIN_BUZZER, LOW);
  }
} //Buzzerul sună o secundă din două — pornit 500ms, oprit 500ms, fără să blocheze restul programului cu delay.

void afiseazaCod() {
  lcd.setCursor(5, 1);
  lcd.print("           ");
  lcd.setCursor(5, 1);
  for (unsigned int i = 0; i < codIntroduced.length(); i++) {
    lcd.print('*');
  }
} //Afișăm pe LCD câte steluțe * corespund cifrelor introduse — pentru securitate, nu afișăm cifrele reale.

void afiseazaStareNormala() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Sistem  ACTIV ");
  lcd.setCursor(0, 1);
  lcd.print("  Supraveghere  ");
} //Afișăm ecranul de repaus când totul e în ordine și sistemul monitorizează.