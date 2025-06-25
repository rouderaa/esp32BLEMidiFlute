#include <Arduino.h>
#include "record.hpp"

void commandSetup()
{
    init_i2s();
}

void commandHandler()
{
    if (Serial.available())
    {
        String filename;
        int ch = Serial.read();
        switch (ch)
        {
        case 'g':
            Serial.printf("Go !\r\n");
            filename = "/recording_" + String(millis()) + ".wav";

            if (record_audio_to_wav(filename.c_str()))
            {
                Serial.println("Recording saved to: " + filename);
            }
            else
            {
                Serial.println("Recording failed!");
            }
            break;
        default:
            Serial.printf("Key: %d %c\r\n", ch, ch);
            break;
        }
    }
}