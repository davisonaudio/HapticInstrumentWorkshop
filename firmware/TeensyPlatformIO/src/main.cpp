#include <Arduino.h>
#include <i2c_device.h>
#include "max98389.h"
#include <Audio.h>
#include <string.h>
#include <limits.h>

#include "au_Biquad.h"
#include "au_config.h"

#include "TransducerFeedbackCancellation.h"
#include "ForceSensing.h"
#include "TeensyEeprom.h"
#include "TeensySerialCommands.h"
#include "MidiComms.h"
#include "au_KarplusStrong.h"
#include "AudioRouting.h"
#include "TeensySlider.h"



//Board revision definitions (only define one):
//#define BOARD_VERSION_REV_A
#define BOARD_VERSION_REV_B



#define BUILD_RELEASE 0 //Set to 1 when generating a release build .hex file

// Write the defined serial number byte to EEPROM when flashing if enabled
// Once done, disable the write serial  to EEPROM and reflash Teensy (avoids the code writing the serial number at every startup).
#define WRITE_SERIAL_NUMBER_TO_FLASH 0
#if WRITE_SERIAL_NUMBER_TO_FLASH
#define TEENSY_SERIAL_NUMBER 12
#endif

//If enabled, this initialises the parameters stored in the EEPROM to their default values.
//Once done, this option should be disabled, the firmware recompiled and reflashed to avoid writing to the EEPROM at every startup (overwriting the existing values).
#define INITIALISE_EEPROM_VALUES 0



#define MAX_SERIAL_INPUT_CHARS 256

#if BUILD_RELEASE
static const unsigned int VERSION_MAJ = 1;
static const unsigned int VERSION_MIN = 4;
#else
//Set version number to 255.255 for debug builds to avoid confusion
static const unsigned int VERSION_MAJ = 255;
static const unsigned int VERSION_MIN = 255;
#endif

const char VERSION_NOTES[] = "Minor fix to include reading of input/output LPF cutoff freqs from eeprom. Default input Fc now 1000Hz.";




IntervalTimer led_blink_timer;

bool configured = false;

char serial_input_buffer[MAX_SERIAL_INPUT_CHARS] = {0};
int input_char_index = 0;




TeensyEeprom teensy_eeprom;
uint8_t serial_number;
TeensySlider teensy_slider;
uint8_t slider_fw_version = 0;

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

extern TransducerFeedbackCancellation::Setup AudioRouting::current_cancellation_setup;
extern ForceSensing AudioRouting::force_sensing;

static const unsigned long LED_BLINK_INTERVAL_NORMAL_OPERATION              =  1000000; //1s
static const unsigned long LED_BLINK_INTERVAL_AMP_NOT_CONFIGURED            =   500000; //500ms
static const unsigned long LED_BLINK_INTERVAL_PLAY_BUFFER_ERROR             =   250000; //250ms
static const unsigned long LED_BLINK_INTERVAL_DEBUG                         =   100000; //100ms
static const unsigned long LED_BLINK_INTERVAL_OTHER                         = 10000000; //10s


void setErrorState(ErrorStates error_state);
void printCurrentTime();
void processSerialInput(char new_char);
void blinkLED();
void readAndApplyEepromParameters();
void writeEepromParameters();
void sendSerialDetails();


// MIDI-related functions
void rxPitchChange(uint8_t channel, int pitch);
void rxProgrammeChange(uint8_t channel, uint8_t programme);
void rxControlChange(uint8_t channel, uint8_t control_number, uint8_t control_value);
void txForceSenseVal(sample_t force_sense_val);

//To reduce latency, set MAX_BUFFERS = 8 in play_queue.h and max_buffers = 8 in record_queue.h


//Pot 1 always controls actuation level, pot 2 controls headphone level


void setup() {

#if WRITE_SERIAL_NUMBER_TO_FLASH
    teensy_eeprom.write(TeensyEeprom::ByteParameters::SERIAL_NUMBER, TEENSY_SERIAL_NUMBER);
#endif

    serial_number = teensy_eeprom.read(TeensyEeprom::ByteParameters::SERIAL_NUMBER);

    // set up Teensy's built in LED
    pinMode(LED_BUILTIN, OUTPUT);
    led_blink_timer.begin(blinkLED, LED_BLINK_INTERVAL_NORMAL_OPERATION);  

    //Setup pin 2 to input due to hardware mod (pin 32 is i2s output and connected to pin 2). DO NOT USE PIN 2 AS AN OUTPUT!!
    pinMode(2, INPUT);

    printf("Teensy has booted.\r\n");
    
    

    AudioRouting::initialiseAudio();


    

    //Setup feedback cancellation
    AudioRouting::resetToDefaultParameters();

#if INITIALISE_EEPROM_VALUES
    writeEepromParameters();
#endif
    
    readAndApplyEepromParameters();

    printf("Resonant frequency: %f\r\n",AudioRouting::current_cancellation_setup.resonant_frequency_hz);

    usbMIDI.setHandlePitchChange(rxPitchChange);
    usbMIDI.setHandleProgramChange(rxProgrammeChange);
    usbMIDI.setHandleControlChange(rxControlChange);

    sendSerialDetails();

    
}


unsigned long total_sample_count = 0;

unsigned long user_controls_time = 0;

void loop() {

    if (AudioRouting::force_sensing.valueAvailable())
    {
        txForceSenseVal(AudioRouting::force_sensing.getDamping());
        //printf("Force sense: %f\r\n",AudioRouting::force_sensing.getDamping());
    }

    //Process USB Serial input (debugging)
    while (Serial.available()) {
        char new_char = Serial.read();
        processSerialInput(new_char);
    }

    //Process USB MIDI input (configuring parameters)
    usbMIDI.read();

    if (user_controls_time < (millis() - 1000))
    {

        // printf("Pot 1 value: %d \r\n",teensy_slider.readPot(1));
        // delayMicroseconds(300);
        // printf("Switch 0: %d \r\n",teensy_slider.getSwitchPressCount(0));
        // delayMicroseconds(300);
        // static uint8_t led_num = 0;
        // teensy_slider.setLedBrightness(led_num, 0);
        // led_num = (led_num + 1) % 10;
        // delayMicroseconds(300);
        // teensy_slider.setLedBrightness(led_num, 255);
        
        user_controls_time = millis();
    }
}

void setErrorState(ErrorStates error_state)
{
    current_error_state = error_state;
    printCurrentTime();
    switch (error_state)
    {
    case ErrorStates::NORMAL_OPERATION:
        printf(" Entering normal operation.\r\n");
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
        AudioRouting::setAudioShieldMode(AudioRouting::AudioShieldMode::DEBUG);
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
    printf("%02d:%02d:%02d", hours, minutes, seconds);
}

void readAndApplyEepromParameters()
{

    sample_t rounded_freq = AudioRouting::force_sensing.setResonantFrequencyHz(teensy_eeprom.read(TeensyEeprom::FloatParameters::RESONANT_FREQUENCY_HZ));
    AudioRouting::force_sensing.setWindowSizePeriods(teensy_eeprom.read(TeensyEeprom::ByteParameters::GOERTZEL_WINDOW_LENGTH));
    AudioRouting::force_sensing.setRawDampedValue(teensy_eeprom.read(TeensyEeprom::FloatParameters::DAMPED_CALIBRATION_VALUE));
    AudioRouting::force_sensing.setRawUndampedValue(teensy_eeprom.read(TeensyEeprom::FloatParameters::UNDAMPED_CALIBRATION_VALUE));

    AudioRouting::current_cancellation_setup.resonant_frequency_hz = rounded_freq;
    AudioRouting::current_cancellation_setup.resonance_peak_gain_db = teensy_eeprom.read(TeensyEeprom::FloatParameters::RESONANT_GAIN_DB);
    AudioRouting::current_cancellation_setup.resonance_q = teensy_eeprom.read(TeensyEeprom::FloatParameters::RESONANT_Q);
    AudioRouting::current_cancellation_setup.resonance_tone_level_db =  teensy_eeprom.read(TeensyEeprom::FloatParameters::TONE_LEVEL_DB);
    AudioRouting::current_cancellation_setup.inductance_filter_coefficient =  teensy_eeprom.read(TeensyEeprom::FloatParameters::INDUCTANCE_FILTER_COEFFICIENT);
    AudioRouting::current_cancellation_setup.transducer_input_wideband_gain_db =  teensy_eeprom.read(TeensyEeprom::FloatParameters::BROADBAND_GAIN_DB);
    AudioRouting::current_cancellation_setup.sample_rate_hz = AUDIO_SAMPLE_RATE_EXACT;
    AudioRouting::current_cancellation_setup.amplifier_type = TransducerFeedbackCancellation::AmplifierType::CURRENT_DRIVE;
    AudioRouting::current_cancellation_setup.lowpass_transducer_io = true;
    AudioRouting::current_cancellation_setup.output_to_transducer_lpf_cutoff_hz = teensy_eeprom.read(TeensyEeprom::FloatParameters::OUTPUT_LPF_CUTOFF_HZ);
    AudioRouting::current_cancellation_setup.input_from_transducer_lpf_cutoff_hz = teensy_eeprom.read(TeensyEeprom::FloatParameters::INPUT_LPF_CUTOFF_HZ);
    AudioRouting::transducer_processing.setup(AudioRouting::current_cancellation_setup);

    AudioRouting::setBoardRevision(teensy_eeprom.readBoardRevision());



    AudioRouting::setHeadphoneLevel(teensy_eeprom.read(TeensyEeprom::FloatParameters::HEADPHONE_LEVEL_DB));
    AudioRouting::setActuationLevel(teensy_eeprom.read(TeensyEeprom::FloatParameters::ACTUATION_LEVEL_DB));

    AudioRouting::setAudioShieldMode(teensy_eeprom.readAudioShieldMode());

    uint8_t stored_maj_version = teensy_eeprom.read(TeensyEeprom::ByteParameters::LAST_SAVED_MAJ_VERSION);
    uint8_t stored_min_version = teensy_eeprom.read(TeensyEeprom::ByteParameters::LAST_SAVED_MIN_VERSION);


    printf("Read parameters from flash. Parameters were stored in V%d.%d. Resonant frequency now: %f\r\n",stored_maj_version, stored_min_version, AudioRouting::current_cancellation_setup.resonant_frequency_hz);
    

}

void writeEepromParameters()
{
    teensy_eeprom.write(TeensyEeprom::FloatParameters::RESONANT_FREQUENCY_HZ, AudioRouting::current_cancellation_setup.resonant_frequency_hz);
    teensy_eeprom.write(TeensyEeprom::FloatParameters::RESONANT_GAIN_DB, AudioRouting::current_cancellation_setup.resonance_peak_gain_db);
    teensy_eeprom.write(TeensyEeprom::FloatParameters::RESONANT_Q, AudioRouting::current_cancellation_setup.resonance_q);
    teensy_eeprom.write(TeensyEeprom::FloatParameters::TONE_LEVEL_DB, AudioRouting::current_cancellation_setup.resonance_tone_level_db);
    teensy_eeprom.write(TeensyEeprom::FloatParameters::INDUCTANCE_FILTER_COEFFICIENT, AudioRouting::current_cancellation_setup.inductance_filter_coefficient);
    teensy_eeprom.write(TeensyEeprom::FloatParameters::BROADBAND_GAIN_DB, AudioRouting::current_cancellation_setup.transducer_input_wideband_gain_db);
    teensy_eeprom.write(TeensyEeprom::FloatParameters::DAMPED_CALIBRATION_VALUE, AudioRouting::force_sensing.getRawDampedValue());
    teensy_eeprom.write(TeensyEeprom::FloatParameters::UNDAMPED_CALIBRATION_VALUE, AudioRouting::force_sensing.getRawUndampedValue());
    teensy_eeprom.write(TeensyEeprom::FloatParameters::OUTPUT_LPF_CUTOFF_HZ, AudioRouting::current_cancellation_setup.output_to_transducer_lpf_cutoff_hz);
    teensy_eeprom.write(TeensyEeprom::FloatParameters::INPUT_LPF_CUTOFF_HZ, AudioRouting::current_cancellation_setup.input_from_transducer_lpf_cutoff_hz);

    teensy_eeprom.write(TeensyEeprom::FloatParameters::HEADPHONE_LEVEL_DB, AudioRouting::getHeadphoneLevel());
    teensy_eeprom.write(TeensyEeprom::FloatParameters::ACTUATION_LEVEL_DB, AudioRouting::getActuationLevel());

    teensy_eeprom.write(TeensyEeprom::ByteParameters::GOERTZEL_WINDOW_LENGTH, AudioRouting::force_sensing.getWindowSizePeriods());
    teensy_eeprom.write(TeensyEeprom::ByteParameters::LAST_SAVED_MAJ_VERSION, VERSION_MAJ);
    teensy_eeprom.write(TeensyEeprom::ByteParameters::LAST_SAVED_MIN_VERSION, VERSION_MIN);
    teensy_eeprom.writeBoardRevision(AudioRouting::BoardRevision::REV_B);
    teensy_eeprom.writeAudioShieldMode(AudioRouting::getAudioShieldMode());
    printCurrentTime();
    printf(" Parameters saved to EEPROM.\r\n");
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

        char* parameter_arg = strtok(serial_input_buffer, " "); //Split input string on space
        char* value_arg = strtok(NULL, " ");

        if (!strncmp(parameter_arg, SerialCommands::kDebugModeString, strlen(SerialCommands::kDebugModeString)))
        {
            setErrorState(ErrorStates::DEBUG);
        }
        else if (!strncmp(parameter_arg, SerialCommands::kNormalModeString, strlen(SerialCommands::kNormalModeString)))
        {
            setErrorState(ErrorStates::NORMAL_OPERATION);
            AudioRouting::setAudioShieldMode(AudioRouting::AudioShieldMode::HEADPHONE_OUTPUT);
        }
        else if (!strncmp(parameter_arg, SerialCommands::kStandaloneSynthModeString, strlen(SerialCommands::kStandaloneSynthModeString)))
        {
            AudioRouting::setAudioShieldMode(AudioRouting::AudioShieldMode::STANDALONE_SYNTH);
            printf("Karplus strong synth enabled\r\n");
        }
        else if (!strncmp(parameter_arg, SerialCommands::kLoopbackTestModeString, strlen(SerialCommands::kLoopbackTestModeString)))
        {
            AudioRouting::setAudioShieldMode(AudioRouting::AudioShieldMode::LOOPBACK_TEST);
            printf("Loopback Test mode enabled\r\n");
        }
        else if (!strncmp(parameter_arg, SerialCommands::kPiezoModeString, strlen(SerialCommands::kPiezoModeString)))
        {
            AudioRouting::setAudioShieldMode(AudioRouting::AudioShieldMode::HP_OP_PIEZO_IP);
            printf("Piezo mode enabled\r\n");
        }
        else if (!strncmp(parameter_arg, SerialCommands::kResetParametersString, strlen(SerialCommands::kResetParametersString)))
        {
            AudioRouting::resetToDefaultParameters();
        }
        else if (!strncmp(parameter_arg, SerialCommands::kSaveToEepromString, strlen(SerialCommands::kSaveToEepromString)))
        {
            writeEepromParameters();
        }
        else if (!strncmp(parameter_arg, SerialCommands::kHelpString, strlen(SerialCommands::kHelpString)))
        {
            printSerialHelp();
        }
        else if (!strncmp(parameter_arg, SerialCommands::kCalibrateDamped, strlen(SerialCommands::kCalibrateDamped)))
        {
            AudioRouting::force_sensing.calibrateDamped();
        }
        else if (!strncmp(parameter_arg, SerialCommands::kCalibrateUndamped, strlen(SerialCommands::kCalibrateUndamped)))
        {
            AudioRouting::force_sensing.calibrateUndamped();
        }
        else if (!strncmp(parameter_arg, SerialCommands::kInfoString, strlen(SerialCommands::kInfoString)))
        {
            sendSerialDetails();
        }
        
        else
        { //Check for arguments that have value parameters
            

            //Check for resonant frequency command
            if (!strncmp(parameter_arg, SerialCommands::kResonantFreqString, strlen(SerialCommands::kResonantFreqString)))
            {
                if (value_arg)
                { //Set the resonant frequency to the provided value
                    AudioRouting::setResonantFrequency(atof(value_arg));
                    printf("Resonant frequency set to: %fHz\r\n", atof(value_arg));
                }
                else
                { //If value_arg = NULL then no value provided, return current value
                    printf("%f\n", AudioRouting::current_cancellation_setup.resonant_frequency_hz);
                }
            }

            //Check for tone level command
            else if (!strncmp(parameter_arg, SerialCommands::kToneLevelString, strlen(SerialCommands::kToneLevelString)))
            {
                if (value_arg)
                { //Set the resonant frequency to the provided value
                    AudioRouting::setToneLevel(atof(value_arg));
                    printf("Tone level set to: %fdB\r\n", atof(value_arg));
                }
                else
                { //If value_arg = NULL then no value provided, return current value
                    printf("%f\n", AudioRouting::current_cancellation_setup.resonance_tone_level_db);
                }
            }


            //Check for resonance q command
            else if (!strncmp(parameter_arg, SerialCommands::kResonantQString, strlen(SerialCommands::kResonantQString)))
            {
                if (value_arg)
                { //Set the resonance q to the provided value
                    AudioRouting::setResonanceQ(atof(value_arg));
                    printf("Resonance q set to: %f\r\n", atof(value_arg));
                }
                else
                { //If value_arg = NULL then no value provided, return current value
                    printf("%f\n", AudioRouting::current_cancellation_setup.resonance_q);
                }
            }

            //Check for resonance gain command
            else if (!strncmp(parameter_arg, SerialCommands::kResonantGainString, strlen(SerialCommands::kResonantGainString)))
            {
                if (value_arg)
                { //Set the resonance q to the provided value
                    AudioRouting::setResonanceGain(atof(value_arg));
                    printf("Resonance gain set to: %fdB\r\n", atof(value_arg));
                }
                else
                { //If value_arg = NULL then no value provided, return current value
                    printf("%f\n", AudioRouting::current_cancellation_setup.resonance_peak_gain_db);
                }
            }

            //Check for wideband gain command
            else if (!strncmp(parameter_arg, SerialCommands::kWidebandGainString, strlen(SerialCommands::kWidebandGainString)))
            {
                if (value_arg)
                { //Set the resonance q to the provided value
                    AudioRouting::setWidebandGain(atof(value_arg));
                    printf("Wideband gain set to: %fdB\r\n", atof(value_arg));
                }
                else
                { //If value_arg = NULL then no value provided, return current value
                    printf("%f\n", AudioRouting::current_cancellation_setup.transducer_input_wideband_gain_db);
                }
            }

            //Check for output lowpass cutoff command
            else if (!strncmp(parameter_arg, SerialCommands::kLowpassOutputFreq, strlen(SerialCommands::kLowpassOutputFreq)))
            {
                if (value_arg)
                { //Set the resonance q to the provided value
                    AudioRouting::setOutputLowpassFc(atof(value_arg));
                    printf("Output lowpass cutoff frequency set to: %fHz\r\n", atof(value_arg));
                }
                else
                { //If value_arg = NULL then no value provided, return current value
                    printf("%f\n", AudioRouting::current_cancellation_setup.output_to_transducer_lpf_cutoff_hz);
                }
            }

            //Check for wideband gain command
            else if (!strncmp(parameter_arg, SerialCommands::kLowpassInputFreq, strlen(SerialCommands::kLowpassInputFreq)))
            {
                if (value_arg)
                { //Set the resonance q to the provided value
                    AudioRouting::setInputLowpassFc(atof(value_arg));
                    printf("Input lowpass cutoff frequency set to: %fHz\r\n", atof(value_arg));
                }
                else
                { //If value_arg = NULL then no value provided, return current value
                    printf("%f\n", AudioRouting::current_cancellation_setup.input_from_transducer_lpf_cutoff_hz);
                }
            }

            else if (!strncmp(parameter_arg, SerialCommands::kInductanceCoefficients, strlen(SerialCommands::kInductanceCoefficients)))
            {
                if (value_arg)
                { //Set the resonance q to the provided value
                    Biquad::Coefficients temp_coefficients;
                    temp_coefficients.a0 = atof(value_arg);
                    bool all_coefficients = true;
                    char* next_val;

                    next_val = strtok(NULL, " ");
                    printf(next_val);
                    printf("\r\n");
                    if (next_val){temp_coefficients.a1 = atof(next_val);} else { all_coefficients = false;}

                    next_val = strtok(NULL, " ");
                    printf(next_val);
                    printf("\r\n");
                    if (next_val){temp_coefficients.a2 = atof(next_val);} else { all_coefficients = false;}

                    next_val = strtok(NULL, " ");
                    printf(next_val);
                    printf("\r\n");
                    if (next_val){temp_coefficients.b1 = atof(next_val);} else { all_coefficients = false;}

                    next_val = strtok(NULL, " ");
                    printf(next_val);
                    printf("\r\n");
                    if (next_val){temp_coefficients.b2 = atof(next_val);} else { all_coefficients = false;}

                    if (all_coefficients)
                    {
                        AudioRouting::setInductanceFilter(temp_coefficients);
                        printf("Inductance filtering set.\r\n");
                    }
                    else
                    {
                        printf("Not all coefficients entered correctly. Try again.\r\n");
                    }
                    
                }
                else
                { //If value_arg = NULL then no value provided, return current value
                    printf("Indutance parameter\r\n");
                }
            }

            //Check for headphone level command
            else if (!strncmp(parameter_arg, SerialCommands::kHeadphoneLevel, strlen(SerialCommands::kHeadphoneLevel)))
            {
                if (value_arg)
                { //Set the resonance q to the provided value
                    AudioRouting::setHeadphoneLevel(atof(value_arg));
                    printf("Headphone level set to: %fdB\r\n", atof(value_arg));
                }
                else
                { //If value_arg = NULL then no value provided, return current value
                    printf("%f\n", AudioRouting::getHeadphoneLevel());
                }
            }

            //Check for actuation level command
            else if (!strncmp(parameter_arg, SerialCommands::kActuationLevel, strlen(SerialCommands::kActuationLevel)))
            {
                if (value_arg)
                { //Set the resonance q to the provided value
                    AudioRouting::setActuationLevel(atof(value_arg));
                    printf("Actuation level set to: %fdB\r\n", atof(value_arg));
                }
                else
                { //If value_arg = NULL then no value provided, return current value
                    printf("%f\n", AudioRouting::getActuationLevel());
                }
            }

            //Check for PCB revision
            else if (!strncmp(parameter_arg, SerialCommands::kRevisionString, strlen(SerialCommands::kRevisionString)))
            {
                if (value_arg)
                { //Set the resonance q to the provided value
                    AudioRouting::setBoardRevision(static_cast<AudioRouting::BoardRevision>(atoi(value_arg)));
                    printf("PCB revision set to: %d (0 = A, 1 = B). Don't forget to save to EEPROM!\r\n", atoi(value_arg));
                }
                else
                { //If value_arg = NULL then no value provided, return current value
                    printf("%d\n", static_cast<int>(AudioRouting::getBoardRevision()));
                }
            }

            else //Catch unrecopnised commands
            {
                //printf("Command not recognised! Type \"help\" to see a list of possible commands\r\n");
            }
           
        }

        input_char_index = 0;
    }
}

void rxPitchChange(uint8_t channel, int pitch)
{
    //Library parses MIDI channels as 1-16. Subtract 1 to make it 0-15.
    channel--;

    // printf("Pitch bend: %f on channel %d\r\n",MidiComms::pitchBendToNormalised(pitch), channel);
    switch (static_cast<MidiComms::PitchBendChannels>(channel))
    {
        case MidiComms::PitchBendChannels::RESONANT_FREQUENCY:
            AudioRouting::setResonantFrequency(pitch - MidiComms::MIN_PITCH_BEND_VALUE);
            break;

        default:
            printf("Undefined pitch bend channel used (%d)\r\n",channel);
            break;
    }
}

void rxProgrammeChange(uint8_t channel, uint8_t programme)
{
    switch (static_cast<MidiComms::ProgrammeChangeTypes>(programme))
    {
        case MidiComms::ProgrammeChangeTypes::SAVE_TO_EEPROM:
            writeEepromParameters();
            break;
        case MidiComms::ProgrammeChangeTypes::RESET_TO_DEFAULT_PARAMETERS:
            AudioRouting::resetToDefaultParameters();
            break;
        case MidiComms::ProgrammeChangeTypes::CALIBRATE_DAMPED:
            AudioRouting::force_sensing.calibrateDamped();
            break;
        case MidiComms::ProgrammeChangeTypes::CALIBRATE_UNDAMPED:
            AudioRouting::force_sensing.calibrateUndamped();
            break;
        default:
            printf("Unknown MIDI programme change (%d) received\r\n", programme);
            break;
    }
}

void rxControlChange(uint8_t channel, uint8_t control_number, uint8_t control_value)
{
    switch (static_cast<MidiComms::ControlChangeTypes>(control_number))
    {
        case MidiComms::ControlChangeTypes::TONE_LEVEL:
            if (current_error_state == ErrorStates::DEBUG)
            {
                printf("MIDI CC - Tone Level: %fdB (raw CC val: %d)\r\n", dBToLin(  ((sample_t) control_value - 127.0)), control_value);
            }
            AudioRouting::transducer_processing.setResonanceToneLevelDb( (sample_t) control_value - 127.0);
            break;
        case MidiComms::ControlChangeTypes::ACTUATION_LEVEL:
            AudioRouting::setActuationLevel((float) control_value - 127.0);
            break;
        case MidiComms::ControlChangeTypes::HEADPHONE_LEVEL:
            AudioRouting::setHeadphoneLevel((float) control_value - 127.0);
            break;
        case MidiComms::ControlChangeTypes::KP_BLEND:
            AudioRouting::setKarplusBlend((float) control_value / 127.0);
            break;
        case MidiComms::ControlChangeTypes::KP_FREQ:
            AudioRouting::setKarplusFreq((float) (control_value + 20) * 10);
            break;
        default:
            printf("Unknown MIDI control change (%d) received\r\n", control_number);
            break;
    }
}

void txForceSenseVal(sample_t force_sense_val)
{
    uint8_t force_sense_byte = static_cast<uint8_t>(127 * force_sense_val);
    usbMIDI.sendControlChange(static_cast<uint8_t>(MidiComms::ControlChangeTypes::TX_FORCE_SENSE), force_sense_byte, 1);
}

void blinkLED() {
    static bool led_state = false;
    led_state = !led_state;
    digitalWrite(LED_BUILTIN, led_state);
}



void sendSerialDetails()
{
    printCurrentTime();
    printf(": Device details:\r\n");
    printf("Serial number: %d\r\n",serial_number);
    printf("Project compiled on %s at %s\r\n",__DATE__, __TIME__);
    printf("Project version %d.%d\r\n", VERSION_MAJ, VERSION_MIN);
    printf("Version notes: %s\r\n",VERSION_NOTES);
    printf("Current resonant frequency: %fHz\r\n",AudioRouting::current_cancellation_setup.resonant_frequency_hz);

    if (AudioRouting::getBoardRevision() == AudioRouting::BoardRevision::REV_A)
    {
        printf("Board version: Rev. A\r\n");
    }
    else if (AudioRouting::getBoardRevision() == AudioRouting::BoardRevision::REV_B)
    {
        printf("Board version: Rev. B\r\n");
    }
    else
    {
        printf("Unknown board version! (flash value: %d)\r\n",static_cast<int>(AudioRouting::getBoardRevision()));
    }
    if (AudioRouting::audioShieldConnected())
    {
        printf("Teensy audio shield is connected\r\n");
    }
    else
    {
        printf("Teensy audio shield not connected.\r\n");
    }
    if (AudioRouting::maxAmpConfigured())
    {
        printf("MAX98389 is configured\r\n");
    }
    else
    {
        printf("MAX98389 is NOT configured. Error in setup!.\r\n");
    }
}
