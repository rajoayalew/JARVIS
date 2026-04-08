#include <Wire.h>
#include "AD5933.h"
#include <math.h>
 
#define START_FREQ  (50000)
#define FREQ_INCR   (0)
#define NUM_INCR    (0)
#define REF_RESIST  (500)
 
double gain[NUM_INCR+1];
int phase[NUM_INCR+1];
 
typedef enum {
  MALE = 0,
  FEMALE = 1,
} gender_t;
 
double weight;
double impedance;
double height;
gender_t user_gender;
 
double initial_rel_TBW;
bool bluetoothConnected = false;
 
void setup(void)
{
  // WARNING: If bluetoothConnected is never updated via an interrupt
  // or a background process, this will cause an infinite loop!
  while (bluetoothConnected == false) {
    // Add logic here to check for bluetooth connection
    // e.g., if (Serial.available()) { bluetoothConnected = true; }
    continue;
  }
 
  // Begin I2C
  Wire.begin();
 
  // Begin serial at 9600 baud for output
  Serial.begin(9600);
  Serial.println("AD5933 Test Started!");
 
  // Perform initial configuration. Fail if any one of these fail.
  if (!(AD5933::reset() &&
        AD5933::setInternalClock(true) &&
        AD5933::setStartFrequency(START_FREQ) &&
        AD5933::setIncrementFrequency(FREQ_INCR) &&
        AD5933::setNumberIncrements(NUM_INCR) &&
        AD5933::setPGAGain(PGA_GAIN_X1)))
        {
            Serial.println("FAILED in initialization!");
            while (true) ;
        }
 
  // Perform calibration sweep
  if (AD5933::calibrate(gain, phase, REF_RESIST, NUM_INCR+1))
    Serial.println("Calibrated!");
  else
    Serial.println("Calibration failed...");
 
  // Easy to use method for frequency sweep
  frequencySweepEasy();
 
  // Declare TBW outside the switch statement to fix scoping errors
  double TBW = 0;
  
  switch (user_gender) {
    case MALE:
      TBW = -3.66 + 0.58 * (pow(height, 1.62) / pow(impedance, 0.7) * (1 / 1.35)) + 0.32 * weight;
      initial_rel_TBW = TBW / weight;
      break;
    case FEMALE:
      TBW = -0.86 + 0.78 * (pow(height, 1.99) / pow(impedance, 0.58) * (1 / 18.91)) + 0.14 * weight;
      initial_rel_TBW = TBW / weight;
      break;
  }
}
 
void loop(void)
{
  // Obtain data about height, weight, gender
  // Code to do this
 
  double rel_TBW;
  double TBW = 0; // Declared outside the switch statement
 
  // Easy to use method for frequency sweep
  frequencySweepEasy();
 
  switch (user_gender) {
    case MALE:
      TBW = -3.66 + 0.58 * (pow(height, 1.62) / pow(impedance, 0.7) * (1 / 1.35)) + 0.32 * weight;
      rel_TBW = TBW / weight;
      break;
    case FEMALE:
      TBW = -0.86 + 0.78 * (pow(height, 1.99) / pow(impedance, 0.58) * (1 / 18.91)) + 0.14 * weight;
      rel_TBW = TBW / weight;
      break;
  }
 
  // Note: Depending on units, rel_TBW might be a small decimal.
  // Make sure a difference of "2" aligns with your expected scale.
  if (rel_TBW - initial_rel_TBW < 2) {
    Serial.print("You are dehydrated\n");
  }
 
  // Delay
  delay(5000);
}
 
// Changed from float32_t to void because it does not return a value
void frequencySweepEasy() {
    // Create arrays to hold the data
    int real[NUM_INCR+1], imag[NUM_INCR+1];
 
    // Perform the frequency sweep
    if (AD5933::frequencySweep(real, imag, NUM_INCR+1)) {
      // Print the frequency data
      int cfreq = START_FREQ/1000;
      for (int i = 0; i < NUM_INCR+1; i++, cfreq += FREQ_INCR/1000) {
        // Print raw frequency data
        Serial.print(cfreq);
        Serial.print(": R=");
        Serial.print(real[i]);
        Serial.print("/I=");
        Serial.print(imag[i]);
 
        // Added 'double' to declare the magnitude variable
        double magnitude = sqrt(pow(real[i], 2) + pow(imag[i], 2));
        impedance = 1/(magnitude*gain[i]);
      }
      Serial.println("Frequency sweep complete!");
    } else {
      Serial.println("Frequency sweep failed...");
    }
}
