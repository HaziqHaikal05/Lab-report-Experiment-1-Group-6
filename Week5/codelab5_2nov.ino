// Motor A pin definitions
const int EN1 = 5; // PWM pin for speed control motor 1
const int EN2 = 6; // PWM pin for speed control motor 2
const int DIR1 = 4; // Direction motor 1
const int DIR2 = 7; // Direction motor 2
void setup() 
{
pinMode(EN1, OUTPUT);
pinMode(EN2, OUTPUT);
pinMode(DIR1, OUTPUT);
pinMode(DIR2, OUTPUT);
}
void loop() 
{
// Move forward
digitalWrite(DIR1, LOW); //clockwise
digitalWrite(DIR2, HIGH);
analogWrite(EN1, 200); // control Speed motor 1: 0-255
analogWrite(EN2, 200); // control Speed motor 2: 0-255
delay(2000);
// Move backward
digitalWrite(DIR1, HIGH); //counterclockwise
digitalWrite(DIR2, LOW);
analogWrite(EN1, 200);
analogWrite(EN2, 200);
delay(2000);
// Stop
analogWrite(EN1, 0);
analogWrite(EN2, 0);
delay(1000);
}