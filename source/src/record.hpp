#include "driver/i2s.h"
#include "SPIFFS.h"
#include <stdint.h>

// Audio configuration
#define SAMPLE_RATE 16000
#define BITS_PER_SAMPLE 24
#define CHANNELS 1
#define RECORD_DURATION_SEC 2
#define BUFFER_SIZE 1024

// I2S configuration
#define I2S_WS 2     // Word Select (LRCLK)
#define I2S_SCK 18   // Serial Clock (BCLK)
#define I2S_SD 4     // Serial Data (DIN)

// WAV file header structure
typedef struct {
    char riff[4];           // "RIFF"
    uint32_t chunk_size;    // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmt_size;      // 16 for PCM
    uint16_t audio_format;  // 1 for PCM
    uint16_t num_channels;  // Number of channels
    uint32_t sample_rate;   // Sample rate
    uint32_t byte_rate;     // Byte rate
    uint16_t block_align;   // Block align
    uint16_t bits_per_sample; // Bits per sample
    char data[4];           // "data"
    uint32_t data_size;     // Data size
} wav_header_t;

void init_i2s() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,  // Use 32-bit for MSM261S4030H0
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,   // Try LEFT first, then RIGHT if needed
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = BUFFER_SIZE,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_0);
    
    // Give I2S time to stabilize
    delay(100);
}

void create_wav_header(wav_header_t* header, uint32_t data_size) {
    // RIFF header
    memcpy(header->riff, "RIFF", 4);
    header->chunk_size = data_size + 36;
    memcpy(header->wave, "WAVE", 4);
    
    // fmt chunk
    memcpy(header->fmt, "fmt ", 4);
    header->fmt_size = 16;
    header->audio_format = 1;  // PCM
    header->num_channels = CHANNELS;
    header->sample_rate = SAMPLE_RATE;
    header->byte_rate = SAMPLE_RATE * CHANNELS * (BITS_PER_SAMPLE / 8);
    header->block_align = CHANNELS * (BITS_PER_SAMPLE / 8);
    header->bits_per_sample = BITS_PER_SAMPLE;
    
    // data chunk
    memcpy(header->data, "data", 4);
    header->data_size = data_size;
}

bool record_audio_to_wav(const char* filename) {
    // Calculate total samples and data size for 24-bit
    uint32_t total_samples = SAMPLE_RATE * RECORD_DURATION_SEC * CHANNELS;
    uint32_t data_size = total_samples * (BITS_PER_SAMPLE / 8);  // 3 bytes per sample for 24-bit
    
    // Allocate full buffer for the entire recording (24-bit data)
    uint8_t* full_audio_buffer = (uint8_t*)malloc(data_size);
    if (!full_audio_buffer) {
        Serial.printf("Failed to allocate %d bytes for full audio buffer", data_size);
        return false;
    }
    
    // Allocate working buffer for I2S reads (I2S reads 32-bit aligned data)
    int32_t* read_buffer = (int32_t*)malloc(BUFFER_SIZE * sizeof(int32_t));
    if (!read_buffer) {
        Serial.println("Failed to allocate read buffer");
        free(full_audio_buffer);
        return false;
    }
    
    Serial.println("Recording started...");
    
    // Clear DMA buffer before recording
    i2s_zero_dma_buffer(I2S_NUM_0);
    
    // Discard first few reads to let I2S stabilize
    size_t dummy_bytes_read;
    for (int i = 0; i < 3; i++) {
        i2s_read(I2S_NUM_0, read_buffer, BUFFER_SIZE * sizeof(int32_t), &dummy_bytes_read, 100);
    }
    
    // Record all audio data into memory buffer first
    uint32_t samples_recorded = 0;
    size_t bytes_read = 0;
    uint32_t buffer_index = 0;
    
    while (samples_recorded < total_samples) {
        // Read from I2S (reads 32-bit aligned data)
        esp_err_t result = i2s_read(I2S_NUM_0, read_buffer, BUFFER_SIZE * sizeof(int32_t), &bytes_read, portMAX_DELAY);
        
        if (result != ESP_OK) {
            Serial.printf("I2S read error: %d\n", result);
            continue;
        }
        
        uint32_t samples_in_buffer = bytes_read / sizeof(int32_t);
        uint32_t samples_to_copy = min(samples_in_buffer, total_samples - samples_recorded);
        
        // Debug: Print first few samples
        // if (samples_recorded < 5) {
        //    Serial.printf("Sample %d: 0x%08X (%d)\n", samples_recorded, read_buffer[0], read_buffer[0]);
        // }
        
        // Convert 32-bit I2S data to 24-bit WAV format for MSM261S4030H0
        for (uint32_t i = 0; i < samples_to_copy; i++) {
            int32_t sample_32bit = read_buffer[i];
            
            // MSM261S4030H0 outputs 24-bit data in the upper 24 bits
            // The data is already sign-extended, so we just need to shift and mask
            int32_t sample_24bit = sample_32bit >> 8;  // Get upper 24 bits
            
            // Ensure we only keep 24 bits (handle sign extension properly)
            sample_24bit = sample_24bit & 0xFFFFFF;
            
            // If the original was negative (MSB of 24-bit data is 1), sign extend properly
            if (sample_24bit & 0x800000) {
                sample_24bit |= 0xFF000000;  // Sign extend for negative numbers
            }
            
            // Store as little-endian 24-bit in the buffer
            full_audio_buffer[buffer_index++] = (sample_24bit) & 0xFF;         // LSB
            full_audio_buffer[buffer_index++] = (sample_24bit >> 8) & 0xFF;    // Middle byte
            full_audio_buffer[buffer_index++] = (sample_24bit >> 16) & 0xFF;   // MSB
        }
        
        samples_recorded += samples_to_copy;
        
        // Progress indicator
        // if (samples_recorded % (SAMPLE_RATE / 4) == 0) {
        //    Serial.print(".");
        // }
    }
    
    Serial.println("\nRecording completed! Writing to file...");
    
    // Now write everything to file at once
    // Create WAV header
    wav_header_t wav_header;
    create_wav_header(&wav_header, data_size);
    
    // Open file for writing
    File file = SPIFFS.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        free(full_audio_buffer);
        free(read_buffer);
        return false;
    }
    
    // Write WAV header
    file.write((uint8_t*)&wav_header, sizeof(wav_header_t));
    
    // Write all audio data at once
    file.write(full_audio_buffer, data_size);
    
    Serial.println("File write completed!");
    
    // Debug: Print some statistics
    Serial.printf("Total samples: %d\n", total_samples);
    Serial.printf("Data size: %d bytes\n", data_size);
    Serial.printf("File size: %d bytes\n", data_size + sizeof(wav_header_t));
    
    // Cleanup
    free(full_audio_buffer);
    free(read_buffer);
    file.close();
    
    return true;
}

void deinit_i2s() {
    i2s_driver_uninstall(I2S_NUM_0);
}

/*
void setup() {
    Serial.begin(115200);
    
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed");
        return;
    }
    
    // Initialize I2S once at startup
    init_i2s();
    
    Serial.println("ESP32 I2S Audio Recorder Ready (24-bit MSM261S4030H0)");
    Serial.println("Call record_audio_to_wav(\"/recording.wav\") to start recording");
}

void loop() {
    // Example usage - record a file every 10 seconds
    static unsigned long last_record = 0;
    
    if (millis() - last_record > 10000) {
        String filename = "/recording_" + String(millis()) + ".wav";
        
        if (record_audio_to_wav(filename.c_str())) {
            Serial.println("Recording saved to: " + filename);
        } else {
            Serial.println("Recording failed!");
        }
        
        last_record = millis();
    }
    
    delay(100);
}
*/