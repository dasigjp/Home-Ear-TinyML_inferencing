
#define EIDSP_QUANTIZE_FILTERBANK   0

#include <Home-Ear-TinyML_inferencing.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include <TFT_eSPI.h> 

TFT_eSPI tft = TFT_eSPI();
#define GRAPH_WIDTH  240
#define GRAPH_HEIGHT 135

/** Audio buffers, pointers and selectors */
typedef struct {
    int16_t *buffer;
    uint8_t buf_ready;
    uint32_t buf_count;
    uint32_t n_samples;
} inference_t;

static inference_t inference;
static const uint32_t sample_buffer_size = 2048;
static signed short sampleBuffer[sample_buffer_size];
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static bool record_status = true;

/**
 * @brief      Arduino setup function
 */
void setup()
{
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(30, 10);
    tft.setTextSize(2);
    Serial.begin(115200);

    // comment out the below line to cancel the wait for USB connection (needed for native USB)
    while (!Serial);
    tft.println("Home Ear Watch");
    Serial.println("Edge Impulse Inferencing Demo");

    tft.setTextSize(1);
    // summary of inferencing settings (from model_metadata.h)
    tft.println("Powering On");
    delay(500);
    tft.println("Initializing Configured Settings.");
    delay(500);
      tft.println("\tBluetooth : Disabled");
      delay(500);
      tft.println("\tWifi configuration : Disabled");
      delay(500);
      tft.println("\tTFT LCD Display : ok");
      delay(500);
      tft.println("\tEsp32 Board     : ok");
      delay(500);
      tft.println("\tinmp41 Status   : ok");
      delay(500);
      tft.println("\tsx1278 Status   : ok");
      delay(500);
      tft.println("\tCoin T motor    : ok");
      delay(500);
      tft.println("\tSystem Status   : ok");
      delay(500);
      tft.println("\tProgram Check   : ok");
      delay(500);
      tft.println("----------------------------------------");
      delay(500);
      tft.println("----------------------------------------");
      delay(500);
      tft.println("----------------------------------------");
      delay(500);

    tft.fillScreen(TFT_BLACK);
    delay(500);
    tft.fillScreen(TFT_WHITE);
    delay(500);
    tft.fillScreen(TFT_RED);
    delay(500);
    tft.fillScreen(TFT_BLUE);
    delay(500);
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 10);
    delay(500);

    tft.println("Inferencing settings:");
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: ");

    tft.print("\tInterval: ");
    tft.println(String((float)EI_CLASSIFIER_INTERVAL_MS) + " ms.");
    ei_printf("\tInterval: %.2f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    delay(500);

    tft.print("\tFrame size: ");
    tft.println(String(EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE));
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    delay(500);

    tft.print("\tSample length: ");
    tft.println(String(EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16) + " ms.");
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    delay(500);

    tft.print("\tNo. of classes: ");
    tft.println(String(sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0])));
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));
    delay(500);

    tft.println(" ");
    delay(500);

    tft.println("EI_CLASSIFIER_CONFIGURATION: ");
    delay(500);

    tft.print("\tINTERVAL in MS: ");
    tft.print(String(EI_CLASSIFIER_INTERVAL_MS));
    tft.println(" ms");
    delay(500);
    Serial.print("EI_CLASSIFIER_INTERVAL_MS: ");
      Serial.println(EI_CLASSIFIER_INTERVAL_MS);
    delay(500);

    tft.print("\tSLICES PER MODEL WINDOW: ");
    tft.print(String(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW));
    tft.println(" Slice");
    Serial.print("EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW: ");
      Serial.println(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);

    tft.println(" ");
    delay(500);

    tft.println("Starting continuous inference in: ");
    delay(500);
    tft.println("-----> 3 seconds");
    delay(1000);
    tft.println("-----> 2 seconds");
    delay(1000);
    tft.println("-----> 1 second");
    delay(1000);

    tft.fillScreen(TFT_BLACK); 

    ei_printf("\nStarting continious inference in 2 seconds...\n");
    ei_sleep(2000);

    if (microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT) == false) {
        tft.println("ERR : Could not allocate audio buffer!");
        delay(500);
        tft.println(String(EI_CLASSIFIER_RAW_SAMPLE_COUNT));
        delay(500);
        tft.println("Please Contact Developer!");
        ei_printf("ERR: Could not allocate audio buffer (size %d), this could be due to the window length of your model\r\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
        return;
    }

    ei_printf("Recording...\n");
    

    
}

/**
 * @brief      Arduino main function. Runs the inferencing loop.
 */
void loop()
{
    bool m = microphone_inference_record();
    if (!m) {
        ei_printf("ERR: Failed to record audio...\n");
        return;
    }

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &microphone_audio_signal_get_data;
    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR r = run_classifier(&signal, &result, debug_nn);
    if (r != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", r);
        return;
    }

    // print the predictions
    ei_printf("Predictions ");
    ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
        result.timing.dsp, result.timing.classification, result.timing.anomaly);
    ei_printf(": \n");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("    %s: ", result.classification[ix].label);
        ei_printf_float(result.classification[ix].value);
        ei_printf("\n");
    }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("    anomaly score: ");
    ei_printf_float(result.anomaly);
    ei_printf("\n");
#endif
    delay(1000); // dinagdag ko
}

static void audio_inference_callback(uint32_t n_bytes)
{
    for(int i = 0; i < n_bytes>>1; i++) {
        inference.buffer[inference.buf_count++] = sampleBuffer[i];

        if(inference.buf_count >= inference.n_samples) {
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
    i2s_read((i2s_port_t)0, (void*)sampleBuffer, i2s_bytes_to_read, &bytes_read, 100);

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
    inference.buffer = (int16_t *)malloc(n_samples * sizeof(int16_t));

    if(inference.buffer == NULL) {
        return false;
    }

    inference.buf_count  = 0;
    inference.n_samples  = n_samples;
    inference.buf_ready  = 0;

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

    while (inference.buf_ready == 0) {
        delay(10);
    }

    inference.buf_ready = 0;
    return ret;
}

/**
 * Get raw audio signal data
 */
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr)
{
    numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);

    return 0;
}

/**
 * @brief      Stop PDM and release buffers
 */
static void microphone_inference_end(void)
{
    i2s_deinit();
    ei_free(inference.buffer);
}


static int i2s_init(uint32_t sampling_rate) {
  // Start listening for audio: MONO @ 8/16KHz
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX ),
      .sample_rate = sampling_rate,
      .bits_per_sample = (i2s_bits_per_sample_t)16,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_I2S,
      .intr_alloc_flags = 0,
      .dma_buf_count = 8,
      .dma_buf_len = 512,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = -1,
  };
  i2s_pin_config_t pin_config = {
      .bck_io_num = 27,    // IIS_SCLK
      .ws_io_num = 26,     // IIS_LCLK
      .data_out_num = -1,  // IIS_DSIN
      .data_in_num = 25,   // IIS_DOUT
  };
  esp_err_t ret = 0;

  ret = i2s_driver_install((i2s_port_t)0, &i2s_config, 0, NULL);
  if (ret != ESP_OK) {
    ei_printf("Error in i2s_driver_install");
  }

  ret = i2s_set_pin((i2s_port_t)0, &pin_config);
  if (ret != ESP_OK) {
    ei_printf("Error in i2s_set_pin");
  }

  ret = i2s_zero_dma_buffer((i2s_port_t)0);
  if (ret != ESP_OK) {
    ei_printf("Error in initializing dma buffer with 0");
  }

  return int(ret);
}

static int i2s_deinit(void) {
    i2s_driver_uninstall((i2s_port_t)0); //stop & destroy i2s driver
    return 0;
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif
