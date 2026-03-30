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

  while (bluetoothConnected == false) {
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

  switch (user_gender) {
    case MALE:
      double TBW = -3.66 + 0.58 * (pow(height, 1.62) / pow(impedance, 0.7) * (1 / 1.35)) + 0.32 * weight;
      initial_rel_TBW = TBW / weight;
    break;
    case FEMALE:
      double TBW = -0.86 + 0.78 * (pow(height, 1.99) / pow(impedance, 0.58) * (1 / 18.91)) + 0.14 * weight;
      initial_rel_TBW = TBW / weight;
    break;
  }
}

void loop(void)
{

  // Obtain data about height, weight, gender
  // Code to do this

  double rel_TBW;

  // Easy to use method for frequency sweep
  frequencySweepEasy();

  switch (user_gender) {
    case MALE:
      double TBW = -3.66 + 0.58 * (pow(height, 1.62) / pow(impedance, 0.7) * (1 / 1.35)) + 0.32 * weight;
      rel_TBW = TBW / weight;
    break;
    case FEMALE:
      double TBW = -0.86 + 0.78 * (pow(height, 1.99) / pow(impedance, 0.58) * (1 / 18.91)) + 0.14 * weight;
      rel_TBW = TBW / weight;
    break;
  }

  if (rel_TBW - initial_rel_TBW < 2) {
    Serial.print("You are dehydrated\n");
  }

  // Delay
  delay(5000);
}

// Easy way to do a frequency sweep. Does an entire frequency sweep at once and
// stores the data into arrays for processing afterwards. This is easy-to-use,
// but doesn't allow you to process data in real time.
float32_t frequencySweepEasy() {
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

        // Compute impedance
        magnitude = sqrt(pow(real[i], 2) + pow(imag[i], 2));
        impedance = 1/(magnitude*gain[i]);
      }
      Serial.println("Frequency sweep complete!");
    } else {
      Serial.println("Frequency sweep failed...");
    }
}

// Removes the frequencySweep abstraction from above. This saves memory and
// allows for data to be processed in real time. However, it's more complex.
void frequencySweepRaw() {
    // Create variables to hold the impedance data and track frequency
    int real, imag, i = 0, cfreq = START_FREQ/1000;

    // Initialize the frequency sweep
    if (!(AD5933::setPowerMode(POWER_STANDBY) &&          // place in standby
          AD5933::setControlMode(CTRL_INIT_START_FREQ) && // init start freq
          AD5933::setControlMode(CTRL_START_FREQ_SWEEP))) // begin frequency sweep
         {
             Serial.println("Could not initialize frequency sweep...");
         }

    // Perform the actual sweep
    while ((AD5933::readStatusRegister() & STATUS_SWEEP_DONE) != STATUS_SWEEP_DONE) {
        // Get the frequency data for this frequency point
        if (!AD5933::getComplexData(&real, &imag)) {
            Serial.println("Could not get raw frequency data...");
        }

        // Print out the frequency data
        Serial.print(cfreq);
        Serial.print(": R=");
        Serial.print(real);
        Serial.print("/I=");
        Serial.print(imag);

        // Compute impedance
        double magnitude = sqrt(pow(real, 2) + pow(imag, 2));
        double impedance = 1/(magnitude*gain[i]);
        Serial.print("  |Z|=");
        Serial.println(impedance);

        // Increment the frequency
        i++;
        cfreq += FREQ_INCR/1000;
        AD5933::setControlMode(CTRL_INCREMENT_FREQ);
    }

    Serial.println("Frequency sweep complete!");

    // Set AD5933 power mode to standby when finished
    if (!AD5933::setPowerMode(POWER_STANDBY))
        Serial.println("Could not set to standby...");
}

