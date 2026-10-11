const int segmentA = 2;
const int segmentB = 3;
const int segmentC = 4;
const int segmentD = 5;
const int segmentE = 6;
const int segmentF = 7;
const int segmentG = 8;

const int inc = 9;
const int reset = 10;

int count = 0;

void setup()
{
  pinMode(segmentA, OUTPUT);
  pinMode(segmentB, OUTPUT);
  pinMode(segmentC, OUTPUT);
  pinMode(segmentD, OUTPUT);
  pinMode(segmentE, OUTPUT);
  pinMode(segmentF, OUTPUT);
  pinMode(segmentG, OUTPUT);

  pinMode(inc, INPUT_PULLUP);
  pinMode(reset, INPUT_PULLUP);
}

void displayNumber(int number)
{
  // Turn everything OFF first
  digitalWrite(segmentA, HIGH);
  digitalWrite(segmentB, HIGH);
  digitalWrite(segmentC, HIGH);
  digitalWrite(segmentD, HIGH);
  digitalWrite(segmentE, HIGH);
  digitalWrite(segmentF, HIGH);
  digitalWrite(segmentG, HIGH);

  if (number == 0)
  {
    digitalWrite(segmentA, LOW);
    digitalWrite(segmentB, LOW);
    digitalWrite(segmentC, LOW);
    digitalWrite(segmentD, LOW);
    digitalWrite(segmentE, LOW);
    digitalWrite(segmentF, LOW);
  }

  else if (number == 1)
  {
    digitalWrite(segmentB, LOW);
    digitalWrite(segmentC, LOW);
  }

  else if (number == 2)
  {
    digitalWrite(segmentA, LOW);
    digitalWrite(segmentB, LOW);
    digitalWrite(segmentD, LOW);
    digitalWrite(segmentE, LOW);
    digitalWrite(segmentG, LOW);
  }

  else if (number == 3)
  {
    digitalWrite(segmentA, LOW);
    digitalWrite(segmentB, LOW);
    digitalWrite(segmentC, LOW);
    digitalWrite(segmentD, LOW);
    digitalWrite(segmentG, LOW);
  }

  else if (number == 4)
  {
    digitalWrite(segmentB, LOW);
    digitalWrite(segmentC, LOW);
    digitalWrite(segmentF, LOW);
    digitalWrite(segmentG, LOW);
  }

  else if (number == 5)
  {
    digitalWrite(segmentA, LOW);
    digitalWrite(segmentC, LOW);
    digitalWrite(segmentD, LOW);
    digitalWrite(segmentF, LOW);
    digitalWrite(segmentG, LOW);
  }

  else if (number == 6)
  {
    digitalWrite(segmentA, LOW);
    digitalWrite(segmentC, LOW);
    digitalWrite(segmentD, LOW);
    digitalWrite(segmentE, LOW);
    digitalWrite(segmentF, LOW);
    digitalWrite(segmentG, LOW);
  }

  else if (number == 7)
  {
    digitalWrite(segmentA, LOW);
    digitalWrite(segmentB, LOW);
    digitalWrite(segmentC, LOW);
  }

  else if (number == 8)
  {
    digitalWrite(segmentA, LOW);
    digitalWrite(segmentB, LOW);
    digitalWrite(segmentC, LOW);
    digitalWrite(segmentD, LOW);
    digitalWrite(segmentE, LOW);
    digitalWrite(segmentF, LOW);
    digitalWrite(segmentG, LOW);
  }

  else if (number == 9)
  {
    digitalWrite(segmentA, LOW);
    digitalWrite(segmentB, LOW);
    digitalWrite(segmentC, LOW);
    digitalWrite(segmentD, LOW);
    digitalWrite(segmentF, LOW);
    digitalWrite(segmentG, LOW);
  }
}

void loop()
{
  displayNumber(count);

  if (digitalRead(inc) == HIGH)
  {
    count++;

     if (count > 9)
    {
      count = 0;
    }

    delay(300);  // debounce
  }

  
  if (digitalRead(reset) == HIGH)
  {
    count = 0;
  
    delay(300);
  }
}
