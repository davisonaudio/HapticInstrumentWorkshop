/*
 
AudioRouting.h
 
Author: Matt Davison
Date: 13/06/2025
 
*/

#pragma once

#include <Audio.h>
#include "max98389.h"
#include <Arduino.h>
#include "au_config.h"
#include "TransducerFeedbackCancellation.h"
#include "au_KarplusStrong.h"
#include "ForceSensing.h"

#define teensy_sample_t int16_t
#define RESONANT_FREQ_HZ 89.0

#define BOARD_VERSION_REV_B

namespace AudioRouting {


AudioInputI2SQuad        i2s_quad_in;
AudioInputUSB            usb_in;

AudioOutputI2SQuad       i2s_quad_out;
AudioOutputUSB           usb_out;


AudioControlSGTL5000     audio_shield;


TransducerFeedbackCancellation transducer_processing;
TransducerFeedbackCancellation::Setup current_cancellation_setup;
ForceSensing force_sensing;
AudioUtils::KarplusStrong kp_synth;
sample_t headphone_level_db = 0.0;
sample_t actuation_level_db = 0.0;


enum class AudioShieldMode
{
    DISCONNECTED = 0, //No audio shield, function as normal to/from USB audio connection
    HEADPHONE_OUTPUT, //Actuation signal also output to headphones (without lowpass applied)
    HP_OP_PIEZO_IP,   //Same as HEADPHONE_OUTPUT but audio input is highpassed and mixed with current return for excitation input (for additional piezo)
    ANALOG_ONLY,      //Audio shield replaces USB connection - audio in is sent to actuation amplifier and current return send to audio out
    STANDALONE_SYNTH,  //No external synth required. Inbuilt resonant synthesis models used and output over headphones.
    LOOPBACK_TEST,    //Loops back usb to usb and analog in to analog out
    DEBUG
};
AudioShieldMode audio_shield_mode;
bool audio_shield_connected = false;
bool max_amp_configured = false;


class AudioRouter : public AudioStream
{
public:
        static constexpr int NUM_INPUTS = 6;
        static constexpr int NUM_OUTPUTS = 5;
        enum class RouterInputs
        {
            AMP_CURRENT,
            AMP_VOLTAGE,
            USB_L,
            USB_R,
            ANALOG_L,
            ANALOG_R
        };
        enum class RouterOutputs
        {
            AMP,
            USB_L,
            USB_R,
            ANALOG_L,
            ANALOG_R
        };        
        AudioRouter() : AudioStream(NUM_INPUTS, inputQueueArray) {
          // any extra initialization
        }
        void update(void){

            //Receive new input buffers for each inputs
            for (int i = 0 ; i < NUM_INPUTS ; i++)
            {
                current_input_queues[i] = receiveWritable(i);
            }

            for (int i = 0 ; i < NUM_OUTPUTS ; i++)
            {
                current_output_queues[i] = allocate();
            }

            /*
             * Process audio here
             */
            //Get User's USB volume setting
            float volume_level = usb_in.volume(); //0.0 - 1.0

            //Loop through each sample in the buffers
            for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {   

                //Convert all incoming samples from int16 to normalised float
                sample_t usb_in_l = getSample(RouterInputs::USB_L, i);
                sample_t usb_in_r = getSample(RouterInputs::USB_R, i);
                sample_t amp_in_voltage = getSample(RouterInputs::AMP_VOLTAGE, i);
                sample_t amp_in_current = getSample(RouterInputs::AMP_CURRENT, i);
                sample_t analog_in_l = getSample(RouterInputs::ANALOG_L, i);
                sample_t analog_in_r = getSample(RouterInputs::ANALOG_R, i);

                //Apply volume level (simple linear scaling currently - could be improved)
                usb_in_l *= volume_level;
                usb_in_r *= volume_level;

                //Cancel actuation signal from sensed signal
                TransducerFeedbackCancellation::UnprocessedSamples unprocessed;
                unprocessed.output_to_transducer = usb_in_l;
                unprocessed.input_from_transducer = amp_in_current; //Current measurement from amp
                unprocessed.reference_input_loopback = amp_in_voltage; //Voltage measurement from amp
                TransducerFeedbackCancellation::ProcessedSamples processed = transducer_processing.process(unprocessed);

                sample_t usb_out_l, usb_out_r, amp_out, analog_out_l, analog_out_r;

                switch (audio_shield_mode)
                {
                case AudioShieldMode::DEBUG:
                    usb_out_l = amp_in_current;
                    usb_out_r = amp_in_voltage;
                    amp_out = usb_in_l;
                    break;
                case AudioShieldMode::LOOPBACK_TEST:
                    usb_out_l = usb_in_l;
                    usb_out_r = usb_in_r;
                    analog_out_l = analog_in_l;
                    analog_out_r = analog_in_r;
                    break;
                case AudioShieldMode::STANDALONE_SYNTH:
                    amp_out = kp_synth.process(processed.input_feedback_removed) * 5;
                    usb_out_l = amp_out;
                    usb_out_l = amp_out;
                    analog_out_l = amp_out;
                    analog_out_r = amp_out;
                    break;
                case AudioShieldMode::HEADPHONE_OUTPUT:
                    analog_out_l = usb_in_l;
                    analog_out_r = usb_in_r;
                    usb_out_l = processed.input_feedback_removed;
                    usb_out_r = processed.input_feedback_removed;
                    amp_out = processed.output_to_transducer;
                    break;
                case AudioShieldMode::DISCONNECTED:
                    usb_out_l = processed.input_feedback_removed;
                    usb_out_r = processed.input_feedback_removed;
                    amp_out = processed.output_to_transducer;
                    break;
                case AudioShieldMode::HP_OP_PIEZO_IP: //TODO: Implement piezo routing?
                    analog_out_l = usb_in_l;
                    analog_out_r = usb_in_r;
                    usb_out_l = processed.input_feedback_removed;
                    usb_out_r = processed.input_feedback_removed;
                    amp_out = processed.output_to_transducer;
                break;

                default:
                    break;
                }

                // force_sensing.process(processed.input_feedback_removed, processed.output_to_transducer);
                force_sensing.process(amp_in_voltage, amp_in_current);

                //Scaling to appropriate level
                amp_out *= dBToLin(actuation_level_db);
                analog_out_l *= dBToLin(headphone_level_db);
                analog_out_r *= dBToLin(headphone_level_db);

                //Send out samples to buffers
                setSample(RouterOutputs::AMP, i, amp_out);
                setSample(RouterOutputs::USB_L, i, usb_out_l);
                setSample(RouterOutputs::USB_R, i, usb_out_r);
                setSample(RouterOutputs::ANALOG_L, i, analog_out_l);
                setSample(RouterOutputs::ANALOG_R, i, analog_out_r);
            }

            //Transmit the buffers to the appropriate outputs and release memory
            for (int i = 0 ; i < NUM_OUTPUTS ; i++)
            {
                if (current_output_queues[i])
                {
                    transmit(current_output_queues[i], i);
                    release(current_output_queues[i]);
                }
            }
            
            //Free the buffers that have been used
            for (int i = 0 ; i < NUM_INPUTS ; i++)
            {
                if (current_input_queues[i])
                {
                    release(current_input_queues[i]);
                }
            }

        }

        void connectInput(RouterInputs input_route, AudioStream &source_object, int source_channel = 0)
        {
            m_input_connections[static_cast<int>(input_route)].connect(source_object, source_channel, *this, static_cast<int>(input_route));
        }

        void connectOutput(RouterOutputs output_route, AudioStream &destination_object, int destination_channel = 0)
        {
            m_output_connections[static_cast<int>(output_route)].connect(*this, static_cast<int>(output_route), destination_object, destination_channel);
        }

private:
        audio_block_t *inputQueueArray[NUM_INPUTS];
        audio_block_t *current_input_queues[NUM_INPUTS];
        audio_block_t *current_output_queues[NUM_OUTPUTS];
        AudioConnection m_input_connections[NUM_INPUTS];
        AudioConnection m_output_connections[NUM_OUTPUTS];

        sample_t getSample(RouterInputs input_type, int index)
        {
            audio_block_t *buffer_pointer = current_input_queues[static_cast<int>(input_type)];
            if (buffer_pointer)
            {
                return intToNormalised<teensy_sample_t>(buffer_pointer->data[index]);
            }
            else
            {
                return 0.0;
            }
        }

        void setSample(RouterOutputs output_type, int index, sample_t value)
        {
            audio_block_t *buffer_pointer = current_output_queues[static_cast<int>(output_type)];
            if (buffer_pointer)
            {
                buffer_pointer->data[index] = normalisedToInt<teensy_sample_t>(value);
            }
            else
            {
                buffer_pointer->data[index] = normalisedToInt<teensy_sample_t>(0.0);
            }
        }

};

AudioRouter audio_router;

void initialiseAudio()
{
        //Configure the Teensy audio shield
    audio_shield_connected = audio_shield.enable();
    audio_shield.volume(1.0);

    if (audio_shield_connected)
    {
        audio_shield_mode = AudioShieldMode::HEADPHONE_OUTPUT;
    }
    else
    {
        audio_shield_mode = AudioShieldMode::DISCONNECTED;
    }

    //Configure amp IC over i2c
    max98389 max;
    max.begin(400 * 1000U);
    // Check that we can see the sensor and configure it.
    max_amp_configured = max.configure();
    if (max_amp_configured) {
        Serial.println("Amplifer chip successfully configured");
    } else {
        Serial.println("Error! Amplifier chip not successfully configured.");
    }

    force_sensing.setup();
    kp_synth.setFrequency(161);

    AudioMemory(128);

    audio_router.connectInput(AudioRouter::RouterInputs::AMP_CURRENT, i2s_quad_in, 2);
    audio_router.connectInput(AudioRouter::RouterInputs::AMP_VOLTAGE, i2s_quad_in, 3);
    audio_router.connectInput(AudioRouter::RouterInputs::USB_L, usb_in, 0);
    audio_router.connectInput(AudioRouter::RouterInputs::USB_R, usb_in, 1);
    audio_router.connectInput(AudioRouter::RouterInputs::ANALOG_L, i2s_quad_in, 0);
    audio_router.connectInput(AudioRouter::RouterInputs::ANALOG_R, i2s_quad_in, 1);

    audio_router.connectOutput(AudioRouter::RouterOutputs::AMP, i2s_quad_out, 2);
    audio_router.connectOutput(AudioRouter::RouterOutputs::USB_L, usb_out, 0);
    audio_router.connectOutput(AudioRouter::RouterOutputs::USB_R, usb_out, 1);
    audio_router.connectOutput(AudioRouter::RouterOutputs::ANALOG_L, i2s_quad_out, 0);
    audio_router.connectOutput(AudioRouter::RouterOutputs::ANALOG_R, i2s_quad_out, 1);


}



void setResonantFrequency(sample_t resonant_frequency_hz)
{
    sample_t rounded_freq = force_sensing.setResonantFrequencyHz(resonant_frequency_hz);
    current_cancellation_setup.resonant_frequency_hz = rounded_freq;
    
    transducer_processing.setResonantFrequencyHz(rounded_freq);
    transducer_processing.setOscillatorFrequencyHz(rounded_freq);
}

void setToneLevel(sample_t tone_level_db)
{
    current_cancellation_setup.resonance_tone_level_db = tone_level_db;
    transducer_processing.setResonanceToneLevelDb(tone_level_db);
}

void setResonanceQ(sample_t resonance_q)
{
    current_cancellation_setup.resonance_q = resonance_q;
    transducer_processing.setResonanceQ(resonance_q);
}

void setWidebandGain(sample_t wideband_gain_db)
{
    current_cancellation_setup.transducer_input_wideband_gain_db = wideband_gain_db;
    transducer_processing.setTransducerInputWidebandGainDb(wideband_gain_db);
}

void setResonanceGain(sample_t resonance_gain_db)
{
    current_cancellation_setup.resonance_peak_gain_db = resonance_gain_db;
    transducer_processing.setResonancePeakGainDb(resonance_gain_db);
}

void setOutputLowpassFc(sample_t cutoff_frequency_hz)
{
    current_cancellation_setup.output_to_transducer_lpf_cutoff_hz = cutoff_frequency_hz;
    transducer_processing.setOutputLpfFrequencyHz(cutoff_frequency_hz);
}

void setInputLowpassFc(sample_t cutoff_frequency_hz)
{
    current_cancellation_setup.input_from_transducer_lpf_cutoff_hz = cutoff_frequency_hz;
    transducer_processing.setInputLpfFrequencyHz(cutoff_frequency_hz);
}

void setHeadphoneLevel(sample_t level_db)
{
    headphone_level_db = auClamp(level_db, -200.0, 0);
}

void setActuationLevel(sample_t level_db)
{
    actuation_level_db = level_db; //auClamp(level_db, -200.0, 0);
}

sample_t getHeadphoneLevel(){return headphone_level_db;}

sample_t getActuationLevel(){return actuation_level_db;}

void setAudioShieldMode(AudioShieldMode mode)
{
    audio_shield_mode = mode;
}
AudioShieldMode getAudioShieldMode(){return audio_shield_mode;}


void resetToDefaultParameters()
{
    current_cancellation_setup.resonant_frequency_hz = RESONANT_FREQ_HZ;
    current_cancellation_setup.resonance_peak_gain_db = -18.3;
    current_cancellation_setup.resonance_q = 10.0;
    current_cancellation_setup.resonance_tone_level_db = -50.0;
    current_cancellation_setup.inductance_filter_coefficient = 0.5;
    current_cancellation_setup.transducer_input_wideband_gain_db = 0.0;
    current_cancellation_setup.sample_rate_hz = AUDIO_SAMPLE_RATE_EXACT;
    current_cancellation_setup.amplifier_type = TransducerFeedbackCancellation::AmplifierType::CURRENT_DRIVE;
    current_cancellation_setup.lowpass_transducer_io = true;
    current_cancellation_setup.output_to_transducer_lpf_cutoff_hz = 10000.0;
    current_cancellation_setup.input_from_transducer_lpf_cutoff_hz = 1000.0;
    transducer_processing.setOscillatorFrequencyHz(RESONANT_FREQ_HZ);
    transducer_processing.setup(current_cancellation_setup);

    force_sensing.setResonantFrequencyHz(RESONANT_FREQ_HZ);
    force_sensing.setWindowSizePeriods(10);

    setHeadphoneLevel(0.0);
    setActuationLevel(0.0);

    printf("Reset parameters to defaults. Resonant frequency now %f\r\n",current_cancellation_setup.resonant_frequency_hz);
}

bool audioShieldConnected()
{
    return audio_shield_connected;
}

bool maxAmpConfigured()
{
    return max_amp_configured;
}


} //Namespace AudioRouting