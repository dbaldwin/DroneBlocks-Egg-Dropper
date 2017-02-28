#include <Wire.h>
#include <Servo.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>


Servo dropServo;

/* Assign a unique ID to this sensor at the same time */
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

int RED = 4;
int GREEN = 5;
int SERVO_CLOSED = 90;
int SERVO_OPEN = 45;

void setup(void) {
  
  Serial.begin(9600);
 
  pinMode(GREEN, OUTPUT);
  pinMode(RED, OUTPUT);

  // Setup the servo and its default position
  dropServo.attach(9);
  delay(100);
  dropServo.write(SERVO_CLOSED);

  if(!mag.begin()) {
    Serial.println("Ooops, no HMC5883 detected ... Check your wiring!");
    while(1);
  }
 
}

/*  We need to determine the rotation direction when it stops rotating
 *  so we can then detect if it made a full rotation
 */
const int CW = 1;
const int CCW = 2;

unsigned long startRotationTime = 0;
int rotationDirection = 0;
float currentHeading = 0;
float previousHeading = 0;
float startHeading = 0;
float tolerance = 5.0; // Allow the sensor to bounce around a bit
float totalRotationDegrees = 0;
unsigned long ledStartTime = 0;


bool isRotating = false;

float headingArray[10];
int headingCount = 0;

void loop(void) {

  /* Reset the LED if it's on */
  if (millis() - ledStartTime > 5000) {

    digitalWrite(GREEN, LOW);
    digitalWrite(RED, LOW);
    dropServo.write(SERVO_CLOSED);
    
  }
  
  /* Get a new sensor event */ 
  sensors_event_t event; 
  mag.getEvent(&event);

  // Hold the module so that Z is pointing 'up' and you can measure the heading with x&y
  // Calculate heading when the magnetometer is level, then correct for signs of axis.
  float heading = atan2(event.magnetic.y, event.magnetic.x);
  
  // Once you have your heading, you must then add your 'Declination Angle', which is the 'Error' of the magnetic field in your location.
  // Find yours here: http://www.magnetic-declination.com/
  // Mine is: -13* 2' W, which is ~13 Degrees, or (which we need) 0.22 radians
  // If you cannot find your Declination, comment out these two lines, your compass will be slightly off.
  float declinationAngle = 0;
  heading += declinationAngle;
  
  // Correct for when signs are reversed.
  if(heading < 0)
    heading += 2*PI;
    
  // Check for wrap due to addition of declination.
  if(heading > 2*PI)
    heading -= 2*PI;

  previousHeading = currentHeading;

  Serial.print("Previous heading: ");
  Serial.print(previousHeading);
  
  // Convert radians to degrees for readability.
  currentHeading = heading * 180/M_PI;

  Serial.print(", Current heading: ");
  Serial.println(currentHeading);

  /* Handle when the compass is done rotating */
  if (abs(currentHeading - previousHeading) <= tolerance) {
   
    if (isRotating) {

      Serial.print("Total rotation degrees: ");

      Serial.println(totalRotationDegrees);

      /*  Based on the rotation direction we need
       *  to calculate and see if a 360 rotation happened.
       *  If a rotation starts at 10 degrees and ends at 30 degrees
       *  we can count on the timer logic to know whether or not a full
       *  rotation happened. For example: a rotation of less than 360 degrees
       *  between 10 and 30 will not fall between the 3 and 5 second filter.
       *  It will happen much faster. This logic can probably be improved.
       */
        
        if ((millis() - startRotationTime > 3000) && (millis() - startRotationTime < 6000)) {
          
          if (rotationDirection == CW && totalRotationDegrees > 340) {

            digitalWrite(GREEN, HIGH);
            digitalWrite(RED, LOW);
            dropServo.write(SERVO_OPEN);
        
          } else if (rotationDirection == CCW && totalRotationDegrees > 340) {

            digitalWrite(GREEN, HIGH);
            digitalWrite(RED, LOW);
            dropServo.write(SERVO_OPEN);
            
          } else {

            digitalWrite(GREEN, LOW);
            digitalWrite(RED, HIGH);
            dropServo.write(SERVO_CLOSED);
            
          }

          // Track when the LED comes on so we can reset it after 5 seconds
          ledStartTime = millis();
          
        }

      startHeading = 0;
      rotationDirection = 0;
      totalRotationDegrees = 0;
      
    }

    isRotating = false;

  /* Handle the CW rotation passing 0 degrees - this will be a negative number like 5 - 355 */
  } else if (rotationDirection == CW && (currentHeading - previousHeading) < 0) {

    totalRotationDegrees += (currentHeading - previousHeading) + 360;

    Serial.println("Passing 0 degrees CW");

  /* Handle the CCW rotation passing 0 degrees - this will be a positive number like 355 - 5 */
  } else if (rotationDirection == CCW && (currentHeading - previousHeading) > 0) {

    Serial.println("Passing 0 degrees CCW");
    totalRotationDegrees += 360 - (currentHeading - previousHeading);
    
  /* Handle the clockwise rotation */
  } else if (currentHeading > previousHeading) {
  
    // Rotation has begun
    if (!isRotating) {
      
      startHeading = currentHeading;
      isRotating = true;
      rotationDirection = CW;
      startRotationTime = millis();

      Serial.print("Starting rotation at: ");
      Serial.println(startHeading);

    // Let's calculating the aggregrate rotation degrees
    } else {
      
      totalRotationDegrees += (abs(currentHeading - previousHeading));
      
    }

  /* Handle the counter clockwise rotation */
  } else if (currentHeading < previousHeading) {
    
    // Rotation has begun
    if (!isRotating) {
      
      startHeading = currentHeading;
      isRotating = true;
      rotationDirection = CCW;
      startRotationTime = millis();

      Serial.print("Starting rotation at: ");
      Serial.println(startHeading);
      
    } else {

      totalRotationDegrees += (abs(currentHeading - previousHeading));
      
    }

  }

  // This will constantly print the heading for debug purposes
  //Serial.println(currentHeading);
  
  delay(250);
}
