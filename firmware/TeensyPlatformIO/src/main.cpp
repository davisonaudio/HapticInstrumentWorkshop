#include <Arduino.h>
#include <i2c_device.h>
#include "max98389.h"
#include <Audio.h>
#include <string.h>

#include "au_Biquad.h"
#include "au_config.h"

#include "TransducerFeedbackCancellation.h"
#include "ForceSensing.h"
#include "TeensyEeprom.h"

#define WRITE_SERIAL_NUMBER_TO_EEPROM 0

#if WRITE_SERIAL_NUMBER_TO_EEPROM
#define TEENSY_SERIAL_NUMBER 11
#endif

#define RESONANT_FREQ_HZ 89.0

#define MAX_SERIAL_INPUT_CHARS 256

static const unsigned int VERSION_MAJ = 0;
static const unsigned int VERSION_MIN = 1;
const char VERSION_NOTES[] = "Debug for testing, lowpass disabled. L = V, R = I";


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
AudioOutputI2S2          i2s2_out;
AudioOutputUSB           usb_out;           //xy=819,377
AudioConnection          patchCord1(i2s_quad_in, 2, queue_inL_i2s, 0);
AudioConnection          patchCord2(i2s_quad_in, 3, queue_inR_i2s, 0);
AudioConnection          patchCord3(usb_in, 0, queue_inL_usb, 0);
AudioConnection          patchCord4(usb_in, 1, queue_inR_usb, 0);
AudioConnection          patchCord5(queue_outR_i2s, 0, i2s_out, 1);
AudioConnection          patchCord6(queue_outL_i2s, 0, i2s_out, 0);
AudioConnection          patchCord9(queue_outR_i2s, 0, i2s2_out, 1);
AudioConnection          patchCord10(queue_outL_i2s, 0, i2s2_out, 0);
AudioConnection          patchCord7(queue_outR_usb, 0, usb_out, 1);
AudioConnection          patchCord8(queue_outL_usb, 0, usb_out, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=527,521
// GUItool: end automatically generated code

IntervalTimer led_blink_timer;
int led_state = LOW;
bool configured = false;

char serial_input_buffer[MAX_SERIAL_INPUT_CHARS] = {0};
int input_char_index = 0;

TransducerFeedbackCancellation transducer_processing;
ForceSensing force_sensing;
Biquad meter_filter;
TeensyEeprom teensy_eeprom;
uint8_t serial_number;

//Basic error states that can occur, used for debug prints and LED blink interval.
enum class ErrorStates
{
    NORMAL_OPERATION,
    AMP_NOT_CONFIGURED,
    PLAY_BUFFER_ERROR,
    DEBUG,
    OTHER
};

ErrorStates current_error_state;

static const unsigned long LED_BLINK_INTERVAL_NORMAL_OPERATION              =  1000000; //1s
static const unsigned long LED_BLINK_INTERVAL_AMP_NOT_CONFIGURED            =   500000; //500ms
static const unsigned long LED_BLINK_INTERVAL_PLAY_BUFFER_ERROR             =   250000; //250ms
static const unsigned long LED_BLINK_INTERVAL_DEBUG                         =   100000; //100ms
static const unsigned long LED_BLINK_INTERVAL_OTHER                         = 10000000; //10s


void setErrorState(ErrorStates error_state);
void printCurrentTime();
void processSerialInput(char new_char);

void rxPitchChange(uint8_t channel, int pitch);

//To reduce latency, set MAX_BUFFERS = 8 in play_queue.h and max_buffers = 8 in record_queue.h

void blinkLED() {
    led_state = !led_state;
    digitalWrite(LED_BUILTIN, led_state);
}

void setup() {

#if WRITE_SERIAL_NUMBER_TO_EEPROM
    teensy_eeprom.write(TeensyEeprom::ByteParameters::SERIAL_NUMBER, TEENSY_SERIAL_NUMBER);
#endif

    serial_number = teensy_eeprom.read(TeensyEeprom::ByteParameters::SERIAL_NUMBER);

    // set up Teensy's built in LED
    pinMode(LED_BUILTIN, OUTPUT);
    led_blink_timer.begin(blinkLED, LED_BLINK_INTERVAL_NORMAL_OPERATION);  

    // pinMode(2, INPUT);

    // Enable the serial port for debugging
    Serial.begin(115200);
    printf("Teensy has booted. Serial number: %d\r\n",serial_number);
    printf("Project compiled on %s at %s\r\n",__DATE__, __TIME__);
    printf("Project version %d.%d\r\n", VERSION_MAJ, VERSION_MIN);
    printf("Version notes: %s\r\n",VERSION_NOTES);

    //Configure amp IC over i2c
    max98389 max;
    max.begin(400 * 1000U);
    // Check that we can see the sensor and configure it.
    configured = max.configure();
    if (configured) {
        Serial.println("Amplifer chip successfully configured");
    } else {
        Serial.println("Error! Amplifier chip not successfully configured.");
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
    processing_setup.lowpass_transducer_io = false;
    transducer_processing.setup(processing_setup);

    transducer_processing.setOscillatorFrequencyHz(500.0);

    //Setup force sensing
    force_sensing.setup();
    force_sensing.setRawDebugPrint(false); //Debug printing for calibration
    force_sensing.setResonantFrequencyHz(RESONANT_FREQ_HZ);

    usbMIDI.setHandlePitchChange(rxPitchChange);

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


//         bp_outL_i2s[i] = processed.output_to_transducer + 0.5 ;
//         bp_outR_i2s[i] = processed.output_to_transducer + 0.5 ;
//         bp_outL_usb[i] = buf_inL_i2s[i];//processed.input_feedback_removed;
//         bp_outR_usb[i] = buf_inR_i2s[i];// - buf_inL_i2s[i];

        bp_outL_i2s[i] = processed.output_to_transducer;//tone_gen.process();//processed.output_to_transducer;
        bp_outR_i2s[i] = processed.output_to_transducer;//bp_outL_i2s[i];//processed.output_to_transducer;
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

    //Process USB Serial input (debugging)
    while (Serial.available()) {
        char new_char = Serial.read();
        // printf("%c", new_char);
        processSerialInput(new_char);
    }

    //Process USB MIDI input (configuring parameters)
    usbMIDI.read();


}

void setErrorState(ErrorStates error_state)
{
    current_error_state = error_state;
    printCurrentTime();
    switch (error_state)
    {
    case ErrorStates::NORMAL_OPERATION:
        printf("Entering normal operation.\r\n");
        led_blink_timer.update(LED_BLINK_INTERVAL_NORMAL_OPERATION);
        break;

    case ErrorStates::AMP_NOT_CONFIGURED:
        printf(" Error occurred while attempting to configure the MAX98389 chip via i2c. Check whether Teensy is connected to the board correctly.\r\n");
        led_blink_timer.update(LED_BLINK_INTERVAL_AMP_NOT_CONFIGURED);
        break;
    
    case ErrorStates::PLAY_BUFFER_ERROR:
        printf(" Error occured while playing output buffer queue.\r\n");
        led_blink_timer.update(LED_BLINK_INTERVAL_PLAY_BUFFER_ERROR);
        break;

    case ErrorStates::OTHER:
        printf(" Other (generic) error occurred.\r\n");
        led_blink_timer.update(LED_BLINK_INTERVAL_OTHER);
        break;

    case ErrorStates::DEBUG:
        printf(" Entered debug mode.\r\n");
        led_blink_timer.update(LED_BLINK_INTERVAL_DEBUG);
        break;
    
    default:
        printf(" Unknown error occurred.\r\n");
        led_blink_timer.update(LED_BLINK_INTERVAL_OTHER);
        break;
    }
}

void printCurrentTime()
{
    int time_s = millis() / 1000;
    int hours = time_s / 3600;
    int minutes = (time_s - (hours * 3600)) / 60;
    int seconds = time_s - (hours * 3600) - (minutes * 60);  
    printf("%02d:%02d:%02d",hours,minutes,seconds);
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

void processSerialInput(char new_char)
{
    if (input_char_index == (MAX_SERIAL_INPUT_CHARS - 1))
    {
        printf("Error! Max number of serial input characters (%d) per line exceeded. Input buffer reset.\r\n",MAX_SERIAL_INPUT_CHARS);
        input_char_index = 0;
        return;
    }

    serial_input_buffer[input_char_index++] = new_char;
    if (new_char == '\n')
    {
        if (!strncmp(serial_input_buffer, "debug\n", strlen("debug\n")))
        {
            setErrorState(ErrorStates::DEBUG);
        }
        else if (!strncmp(serial_input_buffer, "normal\n", strlen("normal\n")))
        {
            setErrorState(ErrorStates::NORMAL_OPERATION);
        }

        input_char_index = 0;
    }
}

void rxPitchChange(uint8_t channel, int pitch)
{

}