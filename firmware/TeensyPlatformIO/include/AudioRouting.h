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

AudioRecordQueue         queue_inR_usb;
AudioRecordQueue         queue_inL_usb;
AudioRecordQueue         queue_inL_max98389;
AudioRecordQueue         queue_inR_max98389;
AudioRecordQueue         queue_inL_audio_shield;
AudioRecordQueue         queue_inR_audio_shield;

//Output to MAX98389 amplifier queues
AudioPlayQueue           queue_outR_max98389;
AudioPlayQueue           queue_outL_max98389;
AudioPlayQueue           queue_outR_usb;
AudioPlayQueue           queue_outL_usb;
//Output to teensy audio shield queues
AudioPlayQueue           queue_outR_audio_shield;
AudioPlayQueue           queue_outL_audio_shield;

AudioConnection          patchAmpInL(i2s_quad_in, 2, queue_inL_max98389, 0);
AudioConnection          patchAmpInR(i2s_quad_in, 3, queue_inR_max98389, 0);
AudioConnection          patchUsbInL(usb_in, 0, queue_inL_usb, 0);
AudioConnection          patchUsbInR(usb_in, 1, queue_inR_usb, 0);

#ifdef BOARD_VERSION_REV_B
AudioConnection          patchCord5(queue_outR_max98389, 0, i2s_quad_out, 3);
AudioConnection          patchCord6(queue_outL_max98389, 0, i2s_quad_out, 2);

AudioConnection          patchCord9(queue_outR_audio_shield, 0, i2s_quad_out, 1);
AudioConnection          patchCord10(queue_outL_audio_shield, 0, i2s_quad_out, 0);
#endif

#ifdef BOARD_VERSION_REV_A
AudioConnection          patchCord5(queue_outR_max98389, 0, i2s_quad_out, 1);
AudioConnection          patchCord6(queue_outL_max98389, 0, i2s_quad_out, 0);
AudioConnection          patchCord9(queue_outR_audio_shield, 0, i2s_quad_out, 3);
AudioConnection          patchCord10(queue_outL_audio_shield, 0, i2s_quad_out, 2);
#endif


AudioConnection          patchUsbOutR(queue_outR_usb, 0, usb_out, 1);
AudioConnection          patchUsbOutL(queue_outL_usb, 0, usb_out, 0);

AudioControlSGTL5000     audio_shield;

teensy_sample_t buf_inL_usb[AUDIO_BLOCK_SAMPLES];
teensy_sample_t buf_inR_usb[AUDIO_BLOCK_SAMPLES];
teensy_sample_t buf_inL_i2s[AUDIO_BLOCK_SAMPLES];
teensy_sample_t buf_inR_i2s[AUDIO_BLOCK_SAMPLES];


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
    DEBUG
};
AudioShieldMode audio_shield_mode;
bool audio_shield_connected = false;
bool max_amp_configured = false;


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

    AudioMemory(32);


    //Begin audio buffer queues
    queue_inL_usb.begin();
    queue_inR_usb.begin();
    queue_inL_max98389.begin();
    queue_inR_max98389.begin();
    // queue_inL_audio_shield.begin();
    // queue_inR_audio_shield.begin();
}

void audioRouterProcess()
{
    if (!queue_inL_max98389.available())
    {
        return; //Only proceed when buffers are available
    }

    int16_t *bp_outL_usb, *bp_outR_usb, *bp_outL_i2s, *bp_outR_i2s, *bp_outL_audio_shield, *bp_outR_audio_shield;


    //Copy queue input buffers
    if (queue_inL_usb.available() && queue_inR_usb.available())
    { //This doesn't block on waiting for USB buffers because new buffers will not always be sent if there is no audio output. This would then block the whole programme indefinitely.
        memcpy(buf_inL_usb, queue_inL_usb.readBuffer(), sizeof(teensy_sample_t)*AUDIO_BLOCK_SAMPLES);
        memcpy(buf_inR_usb, queue_inR_usb.readBuffer(), sizeof(teensy_sample_t)*AUDIO_BLOCK_SAMPLES);
        queue_inL_usb.freeBuffer();
        queue_inR_usb.freeBuffer();
    }

    memcpy(buf_inL_i2s, queue_inL_max98389.readBuffer(), sizeof(teensy_sample_t)*AUDIO_BLOCK_SAMPLES);
    memcpy(buf_inR_i2s, queue_inR_max98389.readBuffer(), sizeof(teensy_sample_t)*AUDIO_BLOCK_SAMPLES);
    
    //Free queue input buffers
    queue_inL_max98389.freeBuffer();
    queue_inR_max98389.freeBuffer();

    // Get pointers to "empty" output buffers
    bp_outL_i2s = queue_outL_max98389.getBuffer();
    bp_outR_i2s = queue_outR_max98389.getBuffer();
    bp_outL_usb = queue_outL_usb.getBuffer();
    bp_outR_usb = queue_outR_usb.getBuffer();

    bp_outL_audio_shield = queue_outL_audio_shield.getBuffer();
    bp_outR_audio_shield = queue_outR_audio_shield.getBuffer();

    //Get User's volume setting
    float volume_level = usb_in.volume(); //0.0 - 1.0

    //Loop through each sample in the buffers
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {   

        //Convert all incoming samples from int16 to normalised float
        sample_t usb_in_l = intToNormalised<teensy_sample_t>(buf_inL_usb[i]);
        sample_t usb_in_r = intToNormalised<teensy_sample_t>(buf_inR_usb[i]);
        sample_t amp_in_voltage = intToNormalised<teensy_sample_t>(buf_inL_i2s[i]);
        sample_t amp_in_current = intToNormalised<teensy_sample_t>(buf_inR_i2s[i]);

        //Apply volume level (simple linear scaling currently - could be improved)
        usb_in_l *= volume_level;
        usb_in_r *= volume_level;

        //Cancel actuation signal from sensed signal
        TransducerFeedbackCancellation::UnprocessedSamples unprocessed;
        unprocessed.output_to_transducer = usb_in_l;
        unprocessed.input_from_transducer = amp_in_current; //Current measurement from amp
        unprocessed.reference_input_loopback = amp_in_voltage; //Voltage measurement from amp
        TransducerFeedbackCancellation::ProcessedSamples processed = transducer_processing.process(unprocessed);

        sample_t usb_out_l, usb_out_r, amp_out;
        if (audio_shield_mode == AudioShieldMode::DEBUG)
        {
            usb_out_l = amp_in_current;
            usb_out_r = amp_in_voltage;//processed.input_feedback_removed;
            amp_out = usb_in_l;
        }
        else
        {
            usb_out_l = processed.input_feedback_removed;
            usb_out_r = processed.input_feedback_removed;
            amp_out = processed.output_to_transducer;
        }

        if (audio_shield_mode == AudioShieldMode::STANDALONE_SYNTH)
        {
            amp_out = kp_synth.process(processed.input_feedback_removed) * 5;
            bp_outL_audio_shield[i] = normalisedToInt<teensy_sample_t>(amp_out) * dBToLin(headphone_level_db);
            bp_outR_audio_shield[i] = normalisedToInt<teensy_sample_t>(amp_out) * dBToLin(headphone_level_db);
            usb_out_l = amp_out;
            usb_out_r = amp_out;
        }

        // Convert from normalised float back to int16 and add into output buffers
        bp_outL_i2s[i] = normalisedToInt<teensy_sample_t>(amp_out) * dBToLin(actuation_level_db);
        bp_outR_i2s[i] = normalisedToInt<teensy_sample_t>(amp_out) * dBToLin(actuation_level_db);
        bp_outL_usb[i] = normalisedToInt<teensy_sample_t>(usb_out_l);
        bp_outR_usb[i] = normalisedToInt<teensy_sample_t>(usb_out_r);
        if (audio_shield_mode == AudioShieldMode::HEADPHONE_OUTPUT)
        { //Straight copy of incoming USB audio to headphone output
            bp_outL_audio_shield[i] = normalisedToInt<teensy_sample_t>(usb_in_l) * dBToLin(headphone_level_db);
            bp_outR_audio_shield[i] = normalisedToInt<teensy_sample_t>(usb_in_r) * dBToLin(headphone_level_db);
        }

        // force_sensing.process(processed.input_feedback_removed, processed.output_to_transducer);
        force_sensing.process(amp_in_voltage, amp_in_current);
    }

    // Play output buffers. Retry until success.
    while(queue_outL_max98389.playBuffer()){
        Serial.println("Play MAX98389 left fail.");
    }
    while(queue_outR_max98389.playBuffer()){
        Serial.println("Play MAX98389 right fail.");
    }
    while(queue_outL_usb.playBuffer()){
        Serial.println("Play usb left fail.");
    }
    while(queue_outR_usb.playBuffer()){
        Serial.println("Play usb right fail.");
    }
    while(queue_outL_audio_shield.playBuffer()){
        Serial.println("Play audio shield left fail.");
    }
    while(queue_outR_audio_shield.playBuffer()){
        Serial.println("Play audio shield right fail.");
    }
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