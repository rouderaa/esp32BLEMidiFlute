#include <Arduino.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <SPIFFS.h>
#include <FS.h>
#include <BLEMidi.h>

#include "commander.hpp"
#include "detect.hpp"
#include "midinotes.h"

#define KEY1 32
#define KEY2 19
gpio_num_t KEY1_NUM = GPIO_NUM_32;

#define LEDOFF 0
#define LEDON 1
bool ledvalue;

#define MIDI_A4 69

// Access Point credentials
const char *ap_ssid = "ESP32-FTP-Server";
const char *ap_password = "esp32ftp123";

// Forward function declarations
void handleFTPServer();
void handleFTPCommand(String command);
void setupDataConnection();
void listFiles();
void sendFile(String fileName);
void receiveFile(String fileName);
void displayStorageInfo();
void displayClockInfo();
String formatBytes(size_t bytes);

// FTP server settings
WiFiServer ftpServer(21);
WiFiServer dataServer(20);
WiFiClient ftpClient;
WiFiClient dataClient;

String ftpUser = "esp32";
String ftpPass = "esp32";

// FTP state variables
bool clientConnected = false;
bool dataConnected = false;
String currentDir = "/";
int dataPort = 20;
unsigned long ledtimestamp;

HardwareSerial Serial1(1); // HACK

#include "midi_sounds.h"

void selectProgram(int channel, int program)
{
  // Program Change: 0xC0 + (channel - 1)
  // Note: GM programs are numbered 1-128, but MIDI uses 0-127
  // Serial.write(0xC0 + (channel - 1));  // Program Change on specified channel
  // Serial.write(program - 1);           // Program number (GM 48 becomes 47)
  BLEMidiServer.programChange(channel, program - 1);
}

void setup()
{
  Serial.begin(921600);
  delay(100);

  pinMode(LED, OUTPUT);
  pinMode(KEY1, INPUT_PULLUP);
  pinMode(KEY2, INPUT_PULLUP);
  ledtimestamp = 0L;
  ledvalue = LEDOFF;
  digitalWrite(LED, ledvalue);

  Serial.println("Initializing bluetooth");
  BLEMidiServer.begin("MIDI one-note device");

  // Get and print the BLE MAC address
  BLEAddress bleMac = BLEDevice::getAddress();
  Serial.print("BLE MAC Address: ");
  Serial.println(bleMac.toString().c_str());

  // Blink led to show startup
  digitalWrite(LED, true);
  delay(200);
  digitalWrite(LED, false);

  // SLEEP: Configure wake-up source (wake on LOW - button pressed)
  esp_sleep_enable_ext0_wakeup(KEY1_NUM, 0);

  Serial.println("\n=== ESP32 FTP Server ===");

  // Initialize SPIFFS with formatting if needed
  Serial.println("Initializing SPIFFS...");
  if (!SPIFFS.begin(false))
  {
    Serial.println("SPIFFS mount failed. Attempting to format...");
    if (SPIFFS.format())
    {
      Serial.println("SPIFFS formatted successfully");
      if (SPIFFS.begin(false))
      {
        Serial.println("SPIFFS mounted after formatting");
      }
      else
      {
        Serial.println("SPIFFS mount failed even after formatting");
        return;
      }
    }
    else
    {
      Serial.println("SPIFFS format failed");
      return;
    }
  }
  else
  {
    Serial.println("SPIFFS mounted successfully");
  }

  // Display SPIFFS storage information
  displayStorageInfo();

  // Display clock information
  displayClockInfo();

  // Set up WiFi Access Point
  Serial.println("\nSetting up WiFi Access Point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  IPAddress apIP = WiFi.softAPIP();
  Serial.println("Access Point created successfully!");
  Serial.println("Network Details:");
  Serial.println("  SSID: " + String(ap_ssid));
  Serial.println("  Password: " + String(ap_password));
  Serial.println("  IP Address: " + apIP.toString());

  // Start FTP server
  ftpServer.begin();
  dataServer.begin();

  Serial.println("\nFTP Server started!");
  Serial.println("FTP Connection Details:");
  Serial.println("  Server: " + apIP.toString());
  Serial.println("  Port: 21");
  Serial.println("  Username: esp32");
  Serial.println("  Password: esp32");
  Serial.println("\nConnect to WiFi '" + String(ap_ssid) + "' then use FTP client");
  Serial.println("========================\n");

  commandSetup();

  detect_setup();
}

/*
void blinkLed()
{
  if (ledtimestamp <= millis())
  {
    ledtimestamp = millis() + 1000L;
    ledvalue = !ledvalue;
    digitalWrite(LED, ledvalue);
  }
}
*/

void checkSleep()
{
  // SLEEP: Check sleep button
  if (digitalRead(KEY1) == LOW)
  {
    delay(150); // allow for key bounce
    // wait for release of key
    while (digitalRead(KEY1) == LOW)
    {
    };
    // Fast blink to show going to sleep
    digitalWrite(LED, true);
    delay(100);
    digitalWrite(LED, false);
    delay(100);
    digitalWrite(LED, true);
    delay(100);
    digitalWrite(LED, false);
    delay(100);

    Serial.println("Going to sleep...");
    delay(100); // Brief delay for serial output
    esp_deep_sleep_start();
  }
}

// Function to convert frequency (Hz) to MIDI note number
static int freqToMidi(float frequency)
{
  // Validate input
  if (frequency <= 0)
  {
    return -1; // Invalid frequency
  }

  // MIDI note 69 (A4) = 440 Hz (concert pitch)
  // Formula: MIDI = 69 + 12 * log2(freq / 440)
  // Using natural log: log2(x) = ln(x) / ln(2)

  float midiNote = 69.0 + 12.0 * (log(frequency / 440.0) / log(2.0));

  // Round to nearest integer
  int roundedMidi = (int)(midiNote + 0.5);

  // Validate MIDI range (0-127)
  if (roundedMidi < 0 || roundedMidi > 127)
  {
    return -1; // Out of MIDI range
  }

  return roundedMidi;
}

static float noteFrequency = 0.0;
static bool silenced = false;
static int prevMidiNumber = -1;
static bool bleInitialised = false;

void playNote(float noteFrequency)
{
  if (digitalRead(KEY2) == LOW)
  {
    if (noteFrequency > 0.0)
    {
      int midiNumber = freqToMidi(noteFrequency);
      if ((midiNumber >= C5) && (midiNumber <= 127) && (prevMidiNumber != midiNumber))
      {
        if (BLEMidiServer.isConnected())
        {
          if (!bleInitialised)
          {
            bleInitialised = true;
            selectProgram(0, GM_ROCK_ORGAN);
            // Play opening note
            BLEMidiServer.noteOn(0, MIDI_A4, 127);
            delay(1000);
            BLEMidiServer.noteOff(0, MIDI_A4, 127);
          }

          if (prevMidiNumber > 0)
          {
            BLEMidiServer.noteOff(0, prevMidiNumber, 127);
          }
          BLEMidiServer.noteOn(0, midiNumber, 127);
          Serial.printf("Midi note: %d\r\n", midiNumber);
        }
        prevMidiNumber = midiNumber;
      }
    }
    else
    {
      BLEMidiServer.noteOff(0, prevMidiNumber, 127);
    }
  }
}

void loop()
{
  checkSleep();

  noteFrequency = detect_loop();
  playNote(noteFrequency);

  commandHandler();

  handleFTPServer();
}

void handleFTPServer()
{
  // Check for new FTP clients
  if (ftpServer.hasClient())
  {
    if (!clientConnected)
    {
      ftpClient = ftpServer.available();
      clientConnected = true;
      Serial.println("FTP Client connected");
      ftpClient.println("220 ESP32 FTP Server ready");
    }
    else
    {
      // Close existing connection and accept new one
      Serial.println("New client connecting, closing existing connection");
      ftpClient.stop();
      dataClient.stop();
      clientConnected = false;
      dataConnected = false;

      // Accept new client
      ftpClient = ftpServer.available();
      clientConnected = true;
      Serial.println("FTP Client connected");
      ftpClient.println("220 ESP32 FTP Server ready");
    }
  }

  // Handle FTP client commands
  if (clientConnected && ftpClient.connected())
  {
    if (ftpClient.available())
    {
      String command = ftpClient.readStringUntil('\n');
      command.trim();
      handleFTPCommand(command);
    }
  }
  else if (clientConnected)
  {
    // Client disconnected
    clientConnected = false;
    dataConnected = false;
    ftpClient.stop();
    dataClient.stop();
    Serial.println("FTP Client disconnected");
  }
}

void handleFTPCommand(String command)
{
  Serial.println("FTP Command: " + command);

  String cmd = command.substring(0, command.indexOf(' '));
  String param = "";

  if (command.indexOf(' ') > 0)
  {
    param = command.substring(command.indexOf(' ') + 1);
  }

  cmd.toUpperCase();

  if (cmd == "USER")
  {
    if (param == ftpUser)
    {
      ftpClient.println("331 Password required");
    }
    else
    {
      ftpClient.println("530 Invalid username");
    }
  }
  else if (cmd == "PASS")
  {
    if (param == ftpPass)
    {
      ftpClient.println("230 Login successful");
    }
    else
    {
      ftpClient.println("530 Invalid password");
    }
  }
  else if (cmd == "SYST")
  {
    ftpClient.println("215 ESP32 FTP Server");
  }
  else if (cmd == "PWD")
  {
    ftpClient.println("257 \"" + currentDir + "\" is current directory");
  }
  else if (cmd == "TYPE")
  {
    ftpClient.println("200 Type set");
  }
  else if (cmd == "PASV")
  {
    setupDataConnection();
  }
  else if (cmd == "LIST")
  {
    if (dataConnected)
    {
      listFiles();
      dataClient.stop();
      dataConnected = false;
      ftpClient.println("226 Transfer complete");
    }
    else
    {
      ftpClient.println("425 No data connection");
    }
  }
  else if (cmd == "RETR")
  {
    if (dataConnected && param.length() > 0)
    {
      sendFile(param);
      dataClient.stop();
      dataConnected = false;
      ftpClient.println("226 Transfer complete");
    }
    else
    {
      ftpClient.println("425 No data connection or filename");
    }
  }
  else if (cmd == "STOR")
  {
    if (dataConnected && param.length() > 0)
    {
      receiveFile(param);
      dataClient.stop();
      dataConnected = false;
      ftpClient.println("226 Transfer complete");
    }
    else
    {
      ftpClient.println("425 No data connection or filename");
    }
  }
  else if (cmd == "DELE")
  {
    if (SPIFFS.remove(currentDir + param))
    {
      ftpClient.println("250 File deleted");
    }
    else
    {
      ftpClient.println("550 File not found");
    }
  }
  else if (cmd == "QUIT")
  {
    ftpClient.println("221 Goodbye");
    delay(100); // Give time for response to send
    ftpClient.stop();
    dataClient.stop();
    clientConnected = false;
    dataConnected = false;
    Serial.println("Client disconnected via QUIT command");
  }
  else
  {
    ftpClient.println("502 Command not implemented");
  }
}

void setupDataConnection()
{
  // Close any existing data connection
  if (dataConnected)
  {
    dataClient.stop();
    dataConnected = false;
  }

  // Use passive mode
  IPAddress ip = WiFi.softAPIP();
  int port = 55000 + random(1000); // Use random port to avoid conflicts

  dataServer.stop();
  delay(100);
  dataServer.begin(port);

  // Calculate port bytes for PASV response
  int p1 = port / 256;
  int p2 = port % 256;

  String response = "227 Entering Passive Mode (";
  response += String(ip[0]) + "," + String(ip[1]) + "," + String(ip[2]) + "," + String(ip[3]);
  response += "," + String(p1) + "," + String(p2) + ")";

  ftpClient.println(response);
  Serial.println("Waiting for data connection on port " + String(port));

  // Wait for data connection
  unsigned long timeout = millis() + 10000; // 10 second timeout
  while (millis() < timeout)
  {
    if (dataServer.hasClient())
    {
      dataClient = dataServer.available();
      dataConnected = true;
      Serial.println("Data connection established on port " + String(port));
      break;
    }
    delay(10);
  }

  if (!dataConnected)
  {
    Serial.println("Data connection timeout");
  }
}

void listFiles()
{
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while (file)
  {
    String fileName = file.name();
    int fileSize = file.size();

    String listing = "-rw-r--r-- 1 esp32 esp32 ";
    listing += String(fileSize);
    listing += " Jan 01 12:00 ";
    listing += fileName;
    listing += "\r\n";

    dataClient.print(listing);
    file = root.openNextFile();
  }
}

void sendFile(String fileName)
{
  String fullPath = currentDir + fileName;
  if (fileName.startsWith("/"))
  {
    fullPath = fileName;
  }

  File file = SPIFFS.open(fullPath, "r");
  if (file)
  {
    ftpClient.println("150 Opening data connection");

    while (file.available())
    {
      uint8_t buffer[512];
      int bytesToRead = min(512, (int)file.available());
      int bytesRead = file.read(buffer, bytesToRead);
      dataClient.write(buffer, bytesRead);
    }
    file.close();
    Serial.println("File sent: " + fullPath);
  }
  else
  {
    ftpClient.println("550 File not found");
  }
}

void receiveFile(String fileName)
{
  String fullPath = currentDir + fileName;
  if (fileName.startsWith("/"))
  {
    fullPath = fileName;
  }

  File file = SPIFFS.open(fullPath, "w");
  if (file)
  {
    ftpClient.println("150 Opening data connection");

    unsigned long timeout = millis() + 30000; // 30 second timeout
    while (dataClient.connected() && millis() < timeout)
    {
      if (dataClient.available())
      {
        uint8_t buffer[512];
        int bytesRead = dataClient.read(buffer, 512);
        file.write(buffer, bytesRead);
        timeout = millis() + 30000; // Reset timeout on activity
      }
      delay(1);
    }
    file.close();
    Serial.println("File received: " + fullPath);
  }
  else
  {
    ftpClient.println("550 Cannot create file");
  }
}

void displayStorageInfo()
{
  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();
  size_t freeBytes = totalBytes - usedBytes;

  Serial.println("SPIFFS Storage Information:");
  Serial.println("  Total Space: " + formatBytes(totalBytes));
  Serial.println("  Used Space:  " + formatBytes(usedBytes));
  Serial.println("  Free Space:  " + formatBytes(freeBytes));
  Serial.println("  Usage: " + String((usedBytes * 100) / totalBytes) + "%");

  // List existing files
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  int fileCount = 0;

  Serial.println("Existing files:");
  while (file)
  {
    Serial.println("  " + String(file.name()) + " (" + formatBytes(file.size()) + ")");
    file = root.openNextFile();
    fileCount++;
  }

  if (fileCount == 0)
  {
    Serial.println("  (no files found)");
  }
  else
  {
    Serial.println("  Total files: " + String(fileCount));
  }
}

String formatBytes(size_t bytes)
{
  if (bytes < 1024)
  {
    return String(bytes) + " B";
  }
  else if (bytes < 1024 * 1024)
  {
    return String(bytes / 1024.0, 1) + " KB";
  }
  else if (bytes < 1024 * 1024 * 1024)
  {
    return String(bytes / (1024.0 * 1024.0), 1) + " MB";
  }
  else
  {
    return String(bytes / (1024.0 * 1024.0 * 1024.0), 1) + " GB";
  }
}

void displayClockInfo()
{
  Serial.println("ESP32 Clock Information:");

  // Get CPU frequency
  uint32_t cpuFreq = getCpuFrequencyMhz();
  Serial.println("  CPU Frequency: " + String(cpuFreq) + " MHz");

  // Get APB frequency (peripheral bus)
  uint32_t apbFreq = getApbFrequency();
  Serial.println("  APB Frequency: " + String(apbFreq / 1000000.0, 1) + " MHz");

  // Get XTAL frequency (crystal oscillator)
  uint32_t xtalFreq = getXtalFrequencyMhz();
  Serial.println("  XTAL Frequency: " + String(xtalFreq) + " MHz");

  // Calculate some derived timings
  Serial.println("  Clock cycle time: " + String(1000.0 / cpuFreq, 3) + " ns");
  Serial.println("  APB cycle time: " + String(1000000.0 / apbFreq, 3) + " us");

  // Show which core we're running on
  Serial.println("  Running on Core: " + String(xPortGetCoreID()));

  // Show total number of cores
  Serial.println("  Total CPU Cores: " + String(ESP.getChipCores()));

  // Flash frequency
  Serial.println("  Flash Speed: " + String(ESP.getFlashChipSpeed() / 1000000) + " MHz");

  Serial.println();
}