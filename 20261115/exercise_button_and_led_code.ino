const int LED_PIN = 4;
const int BUTTON_PIN = 2;

void setup()
{
  pinMode(LED_PIN, OUTPUT); 
  pinMode(BUTTON_PIN, INPUT);
  
  Serial.begin(115200);  // baud rate : 115200
}

void loop()
{
  int buttonState = digitalRead(BUTTON_PIN);  // Read pin#2 voltage
  
  if(buttonState == HIGH){  // if button is pressed, LED is ON
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Pressed, LED ON!");  // print
  }
  else {
    digitalWrite(LED_PIN, LOW);
    Serial.println("Not Pressed, LED OFF!");  // print
  }
  
  delay(500);  // delay 0.5sec
}
