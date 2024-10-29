#include <Arduino.h>
#include <i2c_device.h>
#include "max98389.h"
#include <Audio.h>

#include "au_Biquad.h"
#include "au_config.h"

#include "TransducerFeedbackCancellation.h"
#include "ForceSensing.h"

#define RESONANT_FREQ_HZ 380

//Import generated code here to view block diagram https://www.pjrc.com/teensy/gui/
// GUItool: begin automatically generated code
AudioInputI2SQuad        i2s_quad_in;      //xy=316,366
AudioInputUSB            usb_in;           //xy=331,160
AudioRecordQueue         queue_inR_usb;         //xy=487,179
AudioRecordQueue         queue_inL_usb;         //xy=488,146
AudioRecordQueue         queue_inL_i2s;         //xy=490,338
AudioRecordQueue         queue_inR_i2s;         //xy=492,414
AudioPlayQueue           queue_outR_i2s;         //xy=653,182
AudioPlayQueue           queue_outL_i2s;         //xy=654,147
AudioPlayQueue           queue_outR_usb;         //xy=660,410
AudioPlayQueue           queue_outL_usb;         //xy=664,339
AudioOutputI2S           i2s_out;           //xy=814,160
AudioOutputUSB           usb_out;           //xy=819,377
AudioConnection          patchCord1(i2s_quad_in, 2, queue_inL_i2s, 0);
AudioConnection          patchCord2(i2s_quad_in, 3, queue_inR_i2s, 0);
AudioConnection          patchCord3(usb_in, 0, queue_inL_usb, 0);
AudioConnection          patchCord4(usb_in, 1, queue_inR_usb, 0);
AudioConnection          patchCord5(queue_outR_i2s, 0, i2s_out, 1);
AudioConnection          patchCord6(queue_outL_i2s, 0, i2s_out, 0);
AudioConnection          patchCord7(queue_outR_usb, 0, usb_out, 1);
AudioConnection          patchCord8(queue_outL_usb, 0, usb_out, 0);
//AudioConnection          patchCord7(i2s_quad_in, 2, usb_out, 0);
//AudioConnection          patchCord8(i2s_quad_in, 3, usb_out, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=527,521
// GUItool: end automatically generated code

IntervalTimer myTimer;
int ledState = LOW;
bool configured = false;

TransducerFeedbackCancellation transducer_processing;
ForceSensing force_sensing;
Biquad meter_filter;

//To reduce latency, set MAX_BUFFERS = 8 in play_queue.h and max_buffers = 8 in record_queue.h

void blinkLED() {
  if (ledState == LOW) {
    ledState = HIGH;
  } else {
    ledState = LOW;
  }
  digitalWrite(LED_BUILTIN, ledState);
}

void setup() {
    // blinkLED to run every 1 seconds
    pinMode(LED_BUILTIN, OUTPUT);
    myTimer.begin(blinkLED, 1000000);  

    // Enable the serial port for debugging
    Serial.begin(9600);
    Serial.println("Started");

    max98389 max;
    max.begin(400 * 1000U);
    // Check that we can see the sensor and configure it.
    configured = max.configure();
    if (configured) {
        Serial.println("Configured");
    } else {
        Serial.println("Not configured");
    }
    AudioMemory(512);
    sgtl5000_1.enable();
    sgtl5000_1.volume(0.5);


    TransducerFeedbackCancellation::Setup processing_setup;
    processing_setup.resonant_frequency_hz = RESONANT_FREQ_HZ;
    processing_setup.resonance_peak_gain_db = 23.5;
    processing_setup.resonance_q = 16.0;
    processing_setup.resonance_tone_level_db = -10.0;
    processing_setup.inductance_filter_coefficient = 0.5;
    processing_setup.transducer_input_wideband_gain_db = -8.36706445514;
    processing_setup.sample_rate_hz = AUDIO_SAMPLE_RATE_EXACT;
    processing_setup.amplifier_type = TransducerFeedbackCancellation::AmplifierType::VOLTAGE_DRIVE;
    transducer_processing.setup(processing_setup);

    queue_inL_usb.begin();
    queue_inR_usb.begin();
    queue_inL_i2s.begin();
    queue_inR_i2s.begin();
}

int16_t buf_inL_usb[AUDIO_BLOCK_SAMPLES];
int16_t buf_inR_usb[AUDIO_BLOCK_SAMPLES];
int16_t buf_inL_i2s[AUDIO_BLOCK_SAMPLES];
int16_t buf_inR_i2s[AUDIO_BLOCK_SAMPLES];

int numbuf = 0;
int starttime = 0;
void loop() {
    int16_t *bp_outL_usb, *bp_outR_usb, *bp_outL_i2s, *bp_outR_i2s;

    // Wait for all channels to have content
    while (!queue_inL_usb.available() || !queue_inR_usb.available()
        || !queue_inL_i2s.available() || !queue_inR_i2s.available());
    starttime = micros();
    numbuf++;
    //Copy queue input buffers
    memcpy(buf_inL_usb, queue_inL_usb.readBuffer(), sizeof(short)*AUDIO_BLOCK_SAMPLES);
    memcpy(buf_inR_usb, queue_inR_usb.readBuffer(), sizeof(short)*AUDIO_BLOCK_SAMPLES);
    memcpy(buf_inL_i2s, queue_inL_i2s.readBuffer(), sizeof(short)*AUDIO_BLOCK_SAMPLES);
    memcpy(buf_inR_i2s, queue_inR_i2s.readBuffer(), sizeof(short)*AUDIO_BLOCK_SAMPLES);
    
    //Free queue input buffers
    queue_inL_usb.freeBuffer();
    queue_inR_usb.freeBuffer();
    queue_inL_i2s.freeBuffer();
    queue_inR_i2s.freeBuffer();

    // Get pointers to "empty" output buffers
    bp_outL_i2s = queue_outL_i2s.getBuffer();
    bp_outR_i2s = queue_outR_i2s.getBuffer();
    bp_outL_usb = queue_outL_usb.getBuffer();
    bp_outR_usb = queue_outR_usb.getBuffer();

    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {        
        TransducerFeedbackCancellation::UnprocessedSamples unprocessed;
        /*unprocessed.output_to_transducer = audioRead(context, n, INPUT_ACTUATION_SIGNAL_PIN);
        unprocessed.input_from_transducer = audioRead(context, n, INPUT_VOLTAGE_PIN);
        unprocessed.reference_input_loopback = 5 * audioRead(context, n, INPUT_LOOPBACK_PIN);*/
        unprocessed.output_to_transducer = buf_inL_usb[i];
        unprocessed.input_from_transducer = buf_inR_i2s[i]; //Current measurement from amp
        unprocessed.reference_input_loopback = buf_inL_i2s[i]; //Voltage measurement from amp
        TransducerFeedbackCancellation::ProcessedSamples processed = transducer_processing.process(unprocessed);

        /*audioWrite(context, n, OUTPUT_AMP_PIN, processed.output_to_transducer * 0.2);
        audioWrite(context, n, OUTPUT_LOOPBACK_PIN, processed.output_to_transducer * 0.2);
        audioWrite(context, n, OUTPUT_PICKUP_SIGNAL_PIN, processed.input_feedback_removed);*/

        bp_outL_i2s[i] = processed.output_to_transducer;
        bp_outR_i2s[i] = processed.output_to_transducer;
        bp_outL_usb[i] = processed.input_feedback_removed * 10;
        bp_outR_usb[i] = unprocessed.reference_input_loopback;

        //force_sensing.process(processed.input_feedback_removed, processed.output_to_transducer);

        // rectify and filter signal for GUI meter
        /*sample_t input_feedback_removed_rectified = abs(processed.input_feedback_removed);  
        sample_t input_feedback_removed_rectified_lowpass = meter_filter.process(input_feedback_removed_rectified);*/
    }

    // Play output buffers. Retry until success.
    while(queue_outL_i2s.playBuffer()){
        Serial.println("Play i2s left fail.");
    }
    while(queue_outR_i2s.playBuffer()){
        Serial.println("Play i2s right fail.");
    }
    while(queue_outL_usb.playBuffer()){
        Serial.println("Play usb left fail.");
    }
    while(queue_outR_usb.playBuffer()){
        Serial.println("Play usb right fail.");
    }

    if(numbuf >= 1000){
        //Serial.printf("%d microseconds\n",  micros() - starttime);
        numbuf = 0;
    }

}