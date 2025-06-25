#include <Arduino.h>

#include <driver/i2s.h>
#include <arduinoFFT.h>

// I2S Configuration for INMP441
#define I2S_WS 2   // Word Select (LRCLK)
#define I2S_SD 4   // Serial Data (DIN)
#define I2S_SCK 18 // Serial Clock (BCLK)
#define I2S_PORT I2S_NUM_0

#define LED 5

// FFT configuration
#define SAMPLES 512
#define SAMPLING_FREQUENCY 16000 // 16kHz sampling rate

// Create FFT object - CHANGED TO FLOAT
ArduinoFFT<float> FFT = ArduinoFFT<float>();

// Arrays for FFT computation - CHANGED TO FLOAT
float vReal[SAMPLES];
float vImag[SAMPLES];

// I2S buffer
int32_t i2sBuffer[SAMPLES];

// Note names array
const char *noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

// Initialize I2S for INMP441
void initI2S()
{
  i2s_config_t i2s_config = {
      .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLING_FREQUENCY,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // INMP441 outputs 24-bit in 32-bit container
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // Mono microphone
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 4,
      .dma_buf_len = 1024,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0};

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_SCK,
      .ws_io_num = I2S_WS,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_SD};

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
}

// Read I2S samples from INMP441
bool readI2SSamples()
{
  size_t bytes_read = 0;

  // Read samples from I2S
  esp_err_t result = i2s_read(I2S_PORT, i2sBuffer, sizeof(i2sBuffer), &bytes_read, portMAX_DELAY);

  if (result != ESP_OK || bytes_read != sizeof(i2sBuffer))
  {
    Serial.println("Failed to read I2S data");
    return false;
  }

  return true;
}

// Convert I2S 32-bit samples to normalized float values - CHANGED TO FLOAT
void convertI2SToFloat()
{ // Renamed from convertI2SToDouble
  for (int i = 0; i < SAMPLES; i++)
  {
    // INMP441 provides 24-bit data in upper 24 bits of 32-bit word
    // Right-shift by 8 to get 24-bit signed value
    int32_t sample24 = i2sBuffer[i] >> 8;

    // Normalize to range [-1.0, 1.0] - CHANGED DENOMINATOR TO FLOAT LITERAL
    vReal[i] = (float)sample24 / 8388608.0f; // 2^23 for 24-bit signed
    vImag[i] = 0.0f;                         // Clear imaginary part - CHANGED TO FLOAT LITERAL
  }
}

// Function to convert frequency to musical note - PARAMETER CHANGED TO FLOAT
String frequencyToNote(float frequency)
{ // Parameter changed to float
  if (frequency < 80.0f)
    return "Too Low"; // CHANGED TO FLOAT LITERAL
  if (frequency > 5000.0f)
    return "Too High"; // CHANGED TO FLOAT LITERAL

  // A4 = 440 Hz reference
  float A4 = 440.0f; // CHANGED TO FLOAT LITERAL

  // Calculate semitones from A4 - CHANGED TO FLOAT LOG2
  float semitones = 12.0f * log2f(frequency / A4); // Use log2f for float
  int semitonesRounded = round(semitones);

  // Calculate octave and note index
  int octave = 4 + (semitonesRounded + 9) / 12;
  int noteIndex = ((semitonesRounded + 9) % 12 + 12) % 12;

  // Format result - Ensure String conversion handles float correctly
  String result = String(noteNames[noteIndex]) + String(octave);
  result += " (" + String(frequency, 1) + " Hz)";

  return result;
}

// Apply simple high-pass filter to remove DC offset - CHANGED TO FLOAT
float applyHighPassFilter()
{
  static float previousSample = 0.0f; // CHANGED TO FLOAT
  static float previousOutput = 0.0f; // CHANGED TO FLOAT
  const float alpha = 0.95f;          // High-pass filter coefficient - CHANGED TO FLOAT
  float peak = 0.0;

  for (int i = 0; i < SAMPLES; i++)
  {
    float currentSample = vReal[i];
    if (currentSample > peak)
      peak = currentSample;                                           // CHANGED TO FLOAT
    float output = alpha * (previousOutput + currentSample - previousSample); // CHANGED TO FLOAT
    previousOutput = output;
    previousSample = currentSample;
    vReal[i] = output;
  }
  return peak;
}

// Main function to detect musical note from I2S microphone
String detectMusicalNote()
{
  // Read samples from I2S microphone
  if (!readI2SSamples())
  {
    return "I2S Read Error";
  }

  // Convert I2S samples to float array - FUNCTION CALL CHANGED
  convertI2SToFloat();

  // Apply high-pass filter to remove DC offset
  float peak = applyHighPassFilter();
  Serial.printf("Peak: %f ", peak);

  // Apply window function to reduce spectral leakage
  FFT.windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);

  // Compute FFT
  FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);

  // Compute magnitudes
  FFT.complexToMagnitude(vReal, vImag, SAMPLES);

  // Find peak frequency (ignore DC component and very low frequencies)
  float maxMagnitude = 0; // CHANGED TO FLOAT
  int peakIndex = 0;

  // Focus on musical range (80Hz to 5kHz)
  int minIndex = max(1, (int)((80.0f * SAMPLES) / SAMPLING_FREQUENCY));             // CHANGED TO FLOAT LITERAL
  int maxIndex = min(SAMPLES / 2, (int)((5000.0f * SAMPLES) / SAMPLING_FREQUENCY)); // CHANGED TO FLOAT LITERAL

  for (int i = minIndex; i < maxIndex; i++)
  {
    if (vReal[i] > maxMagnitude)
    {
      maxMagnitude = vReal[i];
      peakIndex = i;
    }
  }

  // Check if we found a significant peak
  if (maxMagnitude < 10.0f)
  { // Adjust threshold based on your environment - CHANGED TO FLOAT LITERAL
    Serial.printf("maxMagnitude(1): %f\r\n", maxMagnitude);
    return "No Clear Note";
  }

  // Calculate frequency from peak index - CHANGED TO FLOAT
  float peakFrequency = ((float)peakIndex * SAMPLING_FREQUENCY) / SAMPLES; // CHANGED TO FLOAT CAST AND CALC

  // Apply quadratic interpolation for better frequency resolution - CHANGED TO FLOAT
  if (peakIndex > minIndex && peakIndex < maxIndex - 1)
  {
    float y1 = vReal[peakIndex - 1]; // CHANGED TO FLOAT
    float y2 = vReal[peakIndex];     // CHANGED TO FLOAT
    float y3 = vReal[peakIndex + 1]; // CHANGED TO FLOAT

    float a = (y1 - 2.0f * y2 + y3) / 2.0f; // CHANGED TO FLOAT LITERALS
    float b = (y3 - y1) / 2.0f;             // CHANGED TO FLOAT LITERALS

    if (a != 0.0f)
    {                                                                                   // CHANGED TO FLOAT LITERAL
      float peakOffset = -b / (2.0f * a);                                               // CHANGED TO FLOAT LITERALS
      peakFrequency = ((float)(peakIndex + peakOffset) * SAMPLING_FREQUENCY) / SAMPLES; // CHANGED TO FLOAT CAST AND CALC
    }
  }

  return frequencyToNote(peakFrequency);
}

// Calculate RMS level for volume indication - CHANGED TO FLOAT
float calculateRMS()
{                   // Return type changed to float
  float sum = 0.0f; // CHANGED TO FLOAT
  for (int i = 0; i < SAMPLES; i++)
  {
    sum += vReal[i] * vReal[i];
  }
  // This function should be called *before* complexToMagnitude
  // For demonstration, assuming vReal holds time-domain data.
  // In your loop(), ensure it's called at the right place.
  return sqrtf(sum / SAMPLES); // Use sqrtf for float
}

void detect_setup()
{
  // Initialize I2S
  initI2S();

  Serial.println("Sample Rate: " + String(SAMPLING_FREQUENCY) + " Hz");
  Serial.println("FFT Resolution: " + String((float)SAMPLING_FREQUENCY / SAMPLES, 1) + " Hz per bin");
  Serial.println("Max detectable frequency: " + String(SAMPLING_FREQUENCY / 2) + " Hz");
}

// Returns 0.0 is no note was detected, frequency otherwise
float detect_loop()
{
  // Read samples from I2S microphone (moved here for RMS)
  if (!readI2SSamples())
  {
    Serial.println("I2S Read Error"); // Print error here, return implicitly
    delay(200);
    return(0.0); // Exit loop iteration if read fails
  }

  unsigned long startTime = millis();

  // Convert I2S samples to float array
  convertI2SToFloat();

  // Apply high-pass filter to remove DC offset
  applyHighPassFilter();

  // Calculate volume level BEFORE FFT processing that changes vReal
  float rmsLevel = calculateRMS();                         // CHANGED TO FLOAT return type
  
  // Serial.print("Raw RMS: ");
  // Serial.println(rmsLevel, 6); // Print with high precision
  
  // Apply scale factor; tune for your volume...
  int volumePercent = min(100, (int)(rmsLevel * 150000.0f)); 

  // Now proceed with FFT for note detection
  // Apply window function to reduce spectral leakage
  FFT.windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);

  // Compute FFT
  FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);

  // Compute magnitudes
  FFT.complexToMagnitude(vReal, vImag, SAMPLES);

  // Find peak frequency (ignore DC component and very low frequencies)
  float maxMagnitude = 0; // CHANGED TO FLOAT
  int peakIndex = 0;

  // Focus on musical range (80Hz to 5kHz)
  int minIndex = max(1, (int)((80.0f * SAMPLES) / SAMPLING_FREQUENCY));
  int maxIndex = min(SAMPLES / 2, (int)((5000.0f * SAMPLES) / SAMPLING_FREQUENCY));

  for (int i = minIndex; i < maxIndex; i++)
  {
    if (vReal[i] > maxMagnitude)
    {
      maxMagnitude = vReal[i];
      peakIndex = i;
    }
  }

  float noteFrequency = 0.0;

  // Check if we found a significant peak
  if (maxMagnitude < 0.2f)
  { // Adjust threshold based on your environment
    // Serial.printf("maxMagnitude(2): %f\r\n", maxMagnitude);
    // Serial.print("Note: No Clear Note");
  }
  else
  {
    // Calculate frequency from peak index
    float peakFrequency = ((float)peakIndex * SAMPLING_FREQUENCY) / SAMPLES;

    // Apply quadratic interpolation for better frequency resolution
    if (peakIndex > minIndex && peakIndex < maxIndex - 1)
    {
      float y1 = vReal[peakIndex - 1];
      float y2 = vReal[peakIndex];
      float y3 = vReal[peakIndex + 1];

      float a = (y1 - 2.0f * y2 + y3) / 2.0f;
      float b = (y3 - y1) / 2.0f;

      if (a != 0.0f)
      {
        float peakOffset = -b / (2.0f * a);
        peakFrequency = ((float)(peakIndex + peakOffset) * SAMPLING_FREQUENCY) / SAMPLES;
      }
    }

    unsigned long endTime = millis();

    Serial.print("Note: ");
    Serial.print(frequencyToNote(peakFrequency));
    Serial.print(" v:");
    Serial.print(rmsLevel, 6);
    Serial.print(" t:");
    Serial.print(endTime-startTime, 6);
    Serial.println();

    noteFrequency = peakFrequency;
  }
  return noteFrequency;
}