int s1 = 2;
int s2 = 3;
int s3 = 4;
int s4 = 5;
int s5 = 6;

void setup() {
  Serial.begin(9600);

  pinMode(s1, INPUT);
  pinMode(s2, INPUT);
  pinMode(s3, INPUT);
  pinMode(s4, INPUT);
  pinMode(s5, INPUT);
}

void loop() {

  int v1 = digitalRead(s1);
  int v2 = digitalRead(s2);
  int v3 = digitalRead(s3);
  int v4 = digitalRead(s4);
  int v5 = digitalRead(s5);

  Serial.print(v1); Serial.print(" ");
  Serial.print(v2); Serial.print(" ");
  Serial.print(v3); Serial.print(" ");
  Serial.print(v4); Serial.print(" ");
  Serial.println(v5);

  delay(1000);
}