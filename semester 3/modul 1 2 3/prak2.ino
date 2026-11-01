const int buttonPin = 4; // Pin D2 (GPIO 4) untuk Push Button
const int ledPin = 5;    // Pin D1 (GPIO 5) untuk LED

int buttonState = 0;       // Tempat nyimpen status tombol saat ini
int lastButtonState = LOW; // Tempat nyimpen status tombol sebelumnya
bool ledState = false;     // Tempat nyimpen kondisi LED (false = OFF, true = ON)

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  
  // Pastiin pas awal nyala LED dalam keadaan MATI
  digitalWrite(ledPin, LOW);
  Serial.println("Sistem Sakelar Toggle Siap!");
}

void loop() {
  // Baca status tombol sekarang
  buttonState = digitalRead(buttonPin);

  // Cek apakah tombol baru aja DITEKAN (transisi dari LOW ke HIGH)
  if (buttonState == HIGH && lastButtonState == LOW) {
    // Balik status LED (kalo tadinya false/OFF jadi true/ON, dan sebaliknya)
    ledState = !ledState;
    
    // Terapkan status baru ke fisik LED
    digitalWrite(ledPin, ledState ? HIGH : LOW);
    
    // Cetak status ke Serial Monitor
    if (ledState) {
      Serial.println("Tombol ditekan -> LED ON (Latching)");
    } else {
      Serial.println("Tombol ditekan -> LED OFF (Latching)");
    }

    // DEBOUNCING: Jeda singkat buat nahan pantulan mekanis tombol
    delay(200);
  }

  // Simpen status tombol sekarang buat dibandingin di loop berikutnya
  lastButtonState = buttonState;
}