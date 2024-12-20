#include <Arduino.h>
#include <i2c_device.h>
#include "max98389.h"
#include <Audio.h>

#include "au_Biquad.h"
#include "au_config.h"

#include "TransducerFeedbackCancellation.h"
#include "ForceSensing.h"
#include "TeensyEeprom.h"

#define RESONANT_FREQ_HZ 89.0

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
TeensyEeprom teensy_eeprom;

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

    // Enable the USB serial port for debugging
    Serial.begin(115200);
    Serial.println("Started");

    //Configure amp IC over i2c
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

    //Setup feedback cancellation
    TransducerFeedbackCancellation::Setup processing_setup;
    processing_setup.resonant_frequency_hz = RESONANT_FREQ_HZ;
    processing_setup.resonance_peak_gain_db = -18.3;
    processing_setup.resonance_q = 10.0;
    processing_setup.resonance_tone_level_db = -10.0;
    processing_setup.inductance_filter_coefficient = 0.5;
    processing_setup.transducer_input_wideband_gain_db = 0.0;
    processing_setup.sample_rate_hz = AUDIO_SAMPLE_RATE_EXACT;
    processing_setup.amplifier_type = TransducerFeedbackCancellation::AmplifierType::CURRENT_DRIVE;
    transducer_processing.setup(processing_setup);

    transducer_processing.setOscillatorFrequencyHz(500.0);

    //Setup force sensing
    force_sensing.setup();
    force_sensing.setRawDebugPrint(true); //Debug printing for calibration
    force_sensing.setResonantFrequencyHz(RESONANT_FREQ_HZ);

    queue_inL_usb.begin();
    queue_inR_usb.begin();
    queue_inL_i2s.begin();
    queue_inR_i2s.begin();
}

int16_t buf_inL_usb[AUDIO_BLOCK_SAMPLES];
int16_t buf_inR_usb[AUDIO_BLOCK_SAMPLES];
int16_t buf_inL_i2s[AUDIO_BLOCK_SAMPLES];
int16_t buf_inR_i2s[AUDIO_BLOCK_SAMPLES];

unsigned long total_sample_count = 0;

void loop() {

    static ToneGenerator tone_gen(500.0, 44100, -10.0);

    int16_t *bp_outL_usb, *bp_outR_usb, *bp_outL_i2s, *bp_outR_i2s;

    // Wait for all channels to have content
    while (!queue_inL_usb.available() || !queue_inR_usb.available()
        || !queue_inL_i2s.available() || !queue_inR_i2s.available());


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

    //Get User's volume setting
    float volume_level = usb_in.volume(); //0.0 - 1.0

    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {   

        //Apply volume level (simple linear scaling currently - could be improved)
        buf_inL_usb[i] *= volume_level;
        buf_inR_usb[i] *= volume_level;

        TransducerFeedbackCancellation::UnprocessedSamples unprocessed;

        unprocessed.output_to_transducer = buf_inL_usb[i];
        unprocessed.input_from_transducer = buf_inR_i2s[i]; //Current measurement from amp
        unprocessed.reference_input_loopback = buf_inL_i2s[i]; //Voltage measurement from amp
        TransducerFeedbackCancellation::ProcessedSamples processed = transducer_processing.process(unprocessed);

        bp_outL_i2s[i] = tone_gen.process();//processed.output_to_transducer;
        bp_outR_i2s[i] = bp_outL_i2s[i];//processed.output_to_transducer;
        bp_outL_usb[i] = processed.input_feedback_removed;
        bp_outR_usb[i] = buf_inR_i2s[i] - buf_inL_i2s[i];

        force_sensing.process(processed.input_feedback_removed, processed.output_to_transducer);
        // if (total_sample_count % (int)(AUDIO_SAMPLE_RATE_EXACT / 10) == 0) //10x per second
        // {
        //     Serial.print("Force sense val:");
        //     Serial.println();
        // }
        //TODO: Get force sense reading and send over MIDI (print initially)

        total_sample_count++;
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

}

void readAndApplyEepromParameters()
{
        // sample_t resonant_frequency_hz;
        // sample_t resonance_peak_gain_db;
        // sample_t resonance_q;
        // sample_t resonance_tone_level_db;
        // sample_t inductance_filter_coefficient;
        // sample_t transducer_input_wideband_gain_db;
        // sample_t sample_rate_hz;
        // AmplifierType amplifier_type;
        // bool lowpass_transducer_io = true;
    TransducerFeedbackCancellation::Setup cancellation_setup;
    cancellation_setup.resonant_frequency_hz = teensy_eeprom.read(TeensyEeprom::FloatParameters::RESONANT_FREQUENCY_HZ);

}