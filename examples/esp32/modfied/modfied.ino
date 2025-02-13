
#define EIDSP_QUANTIZE_FILTERBANK   0

#include <Home-Ear-TinyML_inferencing.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <TFT_eSPI.h> 
#include "driver/i2s.h"


TFT_eSPI tft = TFT_eSPI();

#define GRAPH_WIDTH  240
#define GRAPH_HEIGHT 135

/** Audio buffers, pointers and selectors */
typedef struct {
    signed short *buffers[2];
    unsigned char buf_select;
    unsigned char buf_ready;
    unsigned int buf_count;
    unsigned int n_samples;
} inference_t;

static inference_t inference;
static const uint32_t sample_buffer_size = 2048;
static signed short sampleBuffer[sample_buffer_size];
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
static bool record_status = true;

/**
 * @brief      Arduino setup function
 */
void setup() {
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(10, 10);
    tft.setTextSize(1);

    Serial.begin(115200);
    while (!Serial); // Wait for USB connection (for native USB boards)

    // Display Title
    tft.println("Sound Visualization");
    ei_printf("Sound Visualization\n");
    delay(1000);

    tft.println("Edge Impulse Inferencing Demo");
    ei_printf("Edge Impulse Inferencing Demo\n");
    delay(1000);

    // Display Inferencing Settings
    tft.println("Inferencing settings:");
    ei_printf("Inferencing settings:\n");
    delay(1000);

    tft.print("\tInterval: ");
    tft.println(String((float)EI_CLASSIFIER_INTERVAL_MS) + " ms.");
    ei_printf("\tInterval: %.2f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    delay(1000);

    tft.print("\tFrame size: ");
    tft.println(String(EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE));
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    delay(1000);

    tft.print("\tSample length: ");
    tft.println(String(EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16) + " ms.");
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    delay(1000);

    tft.print("\tNo. of classes: ");
    tft.println(String(sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0])));
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));
    delay(1000);

    // Initialize Classifier
    run_classifier_init();
    tft.println("\nStarting continuous inference in 2 seconds...");
    ei_printf("\nStarting continuous inference in 2 seconds...\n");
    delay(2000);

    // Start Microphone Inference
    if (!microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE)) {
        tft.fillScreen(TFT_BLACK);
        tft.println("ERR: Could not allocate audio buffer!");
        ei_printf("ERR: Microphone init failed!\n");
        Serial.println("Microphone inference start FAILED!");  // ✅ Debugging
        // Instead of stopping execution, just delay and continue
        delay(2000);
    }else {

    Serial.println("Microphone inference started successfully!");  // ✅ Debugging
      tft.println("Microphone inference started successfully!"); 
    }

    tft.println("Recording...");
    ei_printf("Recording...\n");
    delay(500);
    tft.fillScreen(TFT_BLACK);
    delay(500);
    tft.fillScreen(TFT_WHITE);
    delay(500);
    tft.fillScreen(TFT_RED);
    delay(500);
    tft.fillScreen(TFT_BLUE);
}

/**
 * @brief      Arduino main function. Runs the inferencing loop.
 */
void loop()
{
  Serial.println("Entering Debugging Checkpoint for mic... ");  // ✅ Debugging. Entered Void Loop
  

    Serial.println("Starting microphone recording!...");  // ✅ Debugging Microphone Starts
    bool m = microphone_inference_record();
    Serial.println("Finished microphone recording!");    // ✅ Debugging Microphone Ends
      if (!m) {
        ei_printf("ERR: Failed to record audio...\n");
        Serial.println("ERR: Failed to record audio...\n");
        return;
      }else {
        ei_printf("Microphone recorded successfully!\n");
        Serial.println("Microphone recorded successfully!\n");
      }
    Serial.println("CHECKPOINT microphone recording Ends....!");    // ✅ Debugging 

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
    signal.get_data = &microphone_audio_signal_get_data;
    ei_impulse_result_t result = {0};

    EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, debug_nn);
    if (r != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", r);
        return;
    }

    if (++print_results >= (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)) {
        tft.fillScreen(TFT_WHITE);  // Clear screen before updating
        tft.setCursor(10, 10);
        tft.setTextSize(2);
        tft.setTextColor(TFT_BLACK);
        tft.println("Detected Sound:");

        int y_position = 40; // Start position for displaying results

        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            tft.setCursor(10, y_position);
            tft.setTextSize(2);
            tft.printf("%s: %.2f%%", result.classification[ix].label, result.classification[ix].value * 100);
            y_position += 20; // Move down for next line
        }

    #if EI_CLASSIFIER_HAS_ANOMALY == 1
        tft.setCursor(10, y_position + 20);
        tft.setTextSize(2);
        tft.printf("Anomaly: %.2f%%", result.anomaly * 100);
    #endif

        print_results = 0;
    }
}


static void audio_inference_callback(uint32_t n_bytes)
{
    for(int i = 0; i < n_bytes>>1; i++) {
        inference.buffers[inference.buf_select][inference.buf_count++] = sampleBuffer[i];

        if(inference.buf_count >= inference.n_samples) {
            inference.buf_select ^= 1;
            inference.buf_count = 0;
            inference.buf_ready = 1;
        }
    }
}

static void capture_samples(void* arg) {

  const int32_t i2s_bytes_to_read = (uint32_t)arg;
  size_t bytes_read = i2s_bytes_to_read;

  while (record_status) {

    /* read data at once from i2s */
    i2s_read((i2s_port_t)1, (void*)sampleBuffer, i2s_bytes_to_read, &bytes_read, 100);

    if (bytes_read <= 0) {
      ei_printf("Error in I2S read : %d", bytes_read);
    }
    else {
        if (bytes_read < i2s_bytes_to_read) {
        ei_printf("Partial I2S read");
        }

        // scale the data (otherwise the sound is too quiet)
        for (int x = 0; x < i2s_bytes_to_read/2; x++) {
            sampleBuffer[x] = (int16_t)(sampleBuffer[x]) * 8;
        }

        if (record_status) {
            audio_inference_callback(i2s_bytes_to_read);
        }
        else {
            break;
        }
    }
  }
  vTaskDelete(NULL);
}

/**
 * @brief      Init inferencing struct and setup/start PDM
 *
 * @param[in]  n_samples  The n samples
 *
 * @return     { description_of_the_return_value }
 */
static bool microphone_inference_start(uint32_t n_samples)
{
    inference.buffers[0] = (signed short *)malloc(n_samples * sizeof(signed short));

    if (inference.buffers[0] == NULL) {
        return false;
    }

    inference.buffers[1] = (signed short *)malloc(n_samples * sizeof(signed short));

    if (inference.buffers[1] == NULL) {
        ei_free(inference.buffers[0]);
        return false;
    }

    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    if (i2s_init(EI_CLASSIFIER_FREQUENCY)) {
        ei_printf("Failed to start I2S!");
    }

    ei_sleep(100);

    record_status = true;

    xTaskCreate(capture_samples, "CaptureSamples", 1024 * 32, (void*)sample_buffer_size, 10, NULL);

    return true;
}

/**
 * @brief      Wait on new data
 *
 * @return     True when finished
 */


static bool microphone_inference_record(void)
{
    bool ret = true;

    if (inference.buf_ready == 1) {
        ei_printf(
            "Error sample buffer overrun. Decrease the number of slices per model window "
            "(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)\n");
        ret = false;
    }

    while (inference.buf_ready == 0) {
        delay(1);
    }

    inference.buf_ready = 0;
    return true;
}

/**
 * Get raw audio signal data
 */
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr)
{
    numpy::int16_to_float(&inference.buffers[inference.buf_select ^ 1][offset], out_ptr, length);

    return 0;
}

/**
 * @brief      Stop PDM and release buffers
 */
static void microphone_inference_end(void)
{
    i2s_deinit();
    ei_free(inference.buffers[0]);
    ei_free(inference.buffers[1]);
}


static int i2s_init(uint32_t sampling_rate) {
  // Start listening for audio: MONO @ 8/16KHz
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
      .sample_rate = sampling_rate,
      .bits_per_sample = (i2s_bits_per_sample_t)16,
      .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
      .communication_format = I2S_COMM_FORMAT_I2S,
      .intr_alloc_flags = 0,
      .dma_buf_count = 8,
      .dma_buf_len = 512,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = -1,
  };
  i2s_pin_config_t pin_config = {
      .bck_io_num   = 27,    // IIS_SCLK
      .ws_io_num    = 26,     // IIS_LCLK
      .data_out_num = -1,  // IIS_DSIN
      .data_in_num  = 25,   // IIS_DOUT
  };
  esp_err_t ret = 0;

  ret = i2s_driver_install((i2s_port_t)1, &i2s_config, 0, NULL);
  if (ret != ESP_OK) {
    ei_printf("Error in i2s_driver_install");
  }

  ret = i2s_set_pin((i2s_port_t)1, &pin_config);
  if (ret != ESP_OK) {
    ei_printf("Error in i2s_set_pin");
  }

  ret = i2s_zero_dma_buffer((i2s_port_t)1);
  if (ret != ESP_OK) {
    ei_printf("Error in initializing dma buffer with 0");
  }

  return int(ret);
}

static int i2s_deinit(void) {
    i2s_driver_uninstall((i2s_port_t)1); //stop & destroy i2s driver
    return 0;
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif