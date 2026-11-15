const int potPin = A2;
const int ledPin = 9;

void setup()
{
  Serial.begin(115200);  // baudrate
}

void loop()
{
  // analogRead : 10bit(0~1023) -> GPIO : 8bit(0~255)
  int value = int(analogRead(potPin) / 1023.0 * 255);
  // int value = map(analogRead(potPin), 0, 1023, 0, 255);  // use MAP function
  
  Serial.println(value);  //print
  analogWrite(ledPin, value);  // Write analog value
  
  delay(50);
}
