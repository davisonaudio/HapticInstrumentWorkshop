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
#include "standardBell.h"

#define teensy_sample_t int16_t
#define RESONANT_FREQ_HZ 89.0

#define BOARD_VERSION_REV_A

namespace AudioRouting {


AudioInputI2SQuad        i2s_quad_in;
AudioInputUSB            usb_in;

AudioOutputI2SQuad       i2s_quad_out;
AudioOutputUSB           usb_out;

standardBell             bell_synth;

AudioRecordQueue         queue_inR_usb;
AudioRecordQueue         queue_inL_usb;
AudioRecordQueue         queue_inL_max98389;
AudioRecordQueue         queue_inR_max98389;
AudioRecordQueue         queue_inL_audio_shield;
AudioRecordQueue         queue_inR_audio_shield;

AudioRecordQueue         bell_output;

//Output to MAX98389 amplifier queues
AudioPlayQueue           queue_outR_max98389;
AudioPlayQueue           queue_outL_max98389;
AudioPlayQueue           queue_outR_usb;
AudioPlayQueue           queue_outL_usb;
//Output to teensy audio shield queues
AudioPlayQueue           queue_outR_audio_shield;
AudioPlayQueue           queue_outL_audio_shield;

AudioPlayQueue           bell_input;


AudioConnection          patchAmpInL(i2s_quad_in, 2, queue_inL_max98389, 0);
AudioConnection          patchAmpInR(i2s_quad_in, 3, queue_inR_max98389, 0);
AudioConnection          patchUsbInL(usb_in, 0, queue_inL_usb, 0);
AudioConnection          patchUsbInR(usb_in, 1, queue_inR_usb, 0);

#ifdef BOARD_VERSION_REV_B

AudioConnection          patchShieldInL(i2s_quad_in, 0, queue_inL_audio_shield, 0);
AudioConnection          patchShieldInR(i2s_quad_in, 1, queue_inR_audio_shield, 0);

AudioConnection          patchCord5(queue_outR_max98389, 0, i2s_quad_out, 3);
AudioConnection          patchCord6(queue_outL_max98389, 0, i2s_quad_out, 2);

AudioConnection          patchCord9(queue_outR_audio_shield, 0, i2s_quad_out, 1);
AudioConnection          patchCord10(queue_outL_audio_shield, 0, i2s_quad_out, 0);

AudioConnection          inToBell(bell_input, 0, bell_synth, 0);
AudioConnection          outFromBell(bell_synth, 0, bell_output, 0);
// AudioConnection          inToBell(bell_input, 0, bell_output, 0);

#endif

#ifdef BOARD_VERSION_REV_A
AudioConnection          patchCord5(queue_outR_max98389, 0, i2s_quad_out, 1);
AudioConnection          patchCord6(queue_outL_max98389, 0, i2s_quad_out, 0);
#endif


AudioConnection          patchUsbOutR(queue_outR_usb, 0, usb_out, 1);
AudioConnection          patchUsbOutL(queue_outL_usb, 0, usb_out, 0);

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
    DEBUG
};
AudioShieldMode audio_shield_mode;
bool audio_shield_connected = false;
bool audioShieldConnected(){return audio_shield_connected;}

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
    max.begin(1000 * 1000U);
    // Check that we can see the sensor and configure it.
    bool configured = max.configure();
    if (configured) {
        Serial.println("Amplifer chip successfully configured");
    } else {
        Serial.println("Error! Amplifier chip not successfully configured.");
    }

    force_sensing.setup();
    kp_synth.setFrequency(89);

    AudioMemory(512);

    int max_bufs = 2;


    //Begin audio buffer queues
    queue_inL_usb.begin();
    queue_inR_usb.begin();
    queue_inL_max98389.begin();
    queue_inR_max98389.begin();
    bell_output.begin();
    if (audio_shield_connected)
    {
        queue_inL_audio_shield.begin();
        queue_inR_audio_shield.begin();
        queue_outL_audio_shield.setMaxBuffers(max_bufs);
        queue_outR_audio_shield.setMaxBuffers(max_bufs);
    }

    queue_outR_max98389.setMaxBuffers(max_bufs);
    queue_outL_max98389.setMaxBuffers(max_bufs);
    queue_outR_usb.setMaxBuffers(max_bufs);
    queue_outL_usb.setMaxBuffers(max_bufs);
    bell_input.setMaxBuffers(max_bufs);
}


teensy_sample_t amp_in_voltage_b[AUDIO_BLOCK_SAMPLES];
teensy_sample_t amp_in_current_b[AUDIO_BLOCK_SAMPLES];
teensy_sample_t actuation_input_l_b[AUDIO_BLOCK_SAMPLES];
teensy_sample_t actuation_input_r_b[AUDIO_BLOCK_SAMPLES];
teensy_sample_t piezo_input_b[AUDIO_BLOCK_SAMPLES];

teensy_sample_t bell_output_b[AUDIO_BLOCK_SAMPLES];

void routeInputBuffers()
{
 
    memcpy(amp_in_voltage_b, queue_inL_max98389.readBuffer(), sizeof(teensy_sample_t)*AUDIO_BLOCK_SAMPLES);
    memcpy(amp_in_current_b, queue_inR_max98389.readBuffer(), sizeof(teensy_sample_t)*AUDIO_BLOCK_SAMPLES);
    

    switch (audio_shield_mode)
    {
        case AudioShieldMode::ANALOG_ONLY:
            memcpy(actuation_input_l_b, queue_inL_audio_shield.readBuffer(), sizeof(teensy_sample_t)*AUDIO_BLOCK_SAMPLES);
            memcpy(actuation_input_r_b, queue_inR_audio_shield.readBuffer(), sizeof(teensy_sample_t)*AUDIO_BLOCK_SAMPLES);
            // queue_inL_audio_shield.freeBuffer();
            // queue_inR_audio_shield.freeBuffer();
            break;
        case AudioShieldMode::STANDALONE_SYNTH:
            memcpy(bell_output_b, bell_output.readBuffer(), sizeof(teensy_sample_t)*AUDIO_BLOCK_SAMPLES);
            break;
        case AudioShieldMode::DEBUG:
        case AudioShieldMode::DISCONNECTED:
        case AudioShieldMode::HEADPHONE_OUTPUT:
        case AudioShieldMode::HP_OP_PIEZO_IP:
            if (queue_inL_usb.available() && queue_inR_usb.available())
            { //This doesn't block on waiting for USB buffers because new buffers will not always be sent if there is no audio output. This would then block the whole programme indefinitely.
                memcpy(actuation_input_l_b, queue_inL_usb.readBuffer(), sizeof(teensy_sample_t)*AUDIO_BLOCK_SAMPLES);
                memcpy(actuation_input_r_b, queue_inR_usb.readBuffer(), sizeof(teensy_sample_t)*AUDIO_BLOCK_SAMPLES);
                // queue_inL_usb.freeBuffer();
                // queue_inR_usb.freeBuffer();
            }
            break;
    }
    if (audio_shield_mode == AudioShieldMode::HP_OP_PIEZO_IP)
    {
        memcpy(piezo_input_b, queue_inL_audio_shield.readBuffer(), sizeof(teensy_sample_t)*AUDIO_BLOCK_SAMPLES);
        // queue_inL_audio_shield.freeBuffer();    
    }

    queue_inL_max98389.freeBuffer();
    queue_inR_max98389.freeBuffer();
    queue_inL_audio_shield.freeBuffer();
    queue_inR_audio_shield.freeBuffer();
    queue_inL_usb.freeBuffer();
    queue_inR_usb.freeBuffer();
    bell_output.freeBuffer();

    
}

void audioRouterProcess()
{
    if (!queue_inL_max98389.available())
    {
        return; //Only proceed when buffers are available
    }

    routeInputBuffers();

    int16_t *bp_outL_usb, *bp_outR_usb, *bp_outL_i2s, *bp_outR_i2s, *bp_outL_audio_shield, *bp_outR_audio_shield, *bp_bell_input;

    // Get pointers to "empty" output buffers
    bp_outL_i2s = queue_outL_max98389.getBuffer();
    bp_outR_i2s = queue_outR_max98389.getBuffer();
    bp_outL_usb = queue_outL_usb.getBuffer();
    bp_outR_usb = queue_outR_usb.getBuffer();

    bp_outL_audio_shield = queue_outL_audio_shield.getBuffer();
    bp_outR_audio_shield = queue_outR_audio_shield.getBuffer();

    bp_bell_input = bell_input.getBuffer();

    

    //Loop through each sample in the buffers
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {   

        //Convert all incoming samples from int16 to normalised float
        sample_t actuation_input_l = intToNormalised<teensy_sample_t>(actuation_input_l_b[i]);
        sample_t actuation_input_r = intToNormalised<teensy_sample_t>(actuation_input_r_b[i]);
        sample_t amp_in_voltage = intToNormalised<teensy_sample_t>(amp_in_voltage_b[i]);
        sample_t amp_in_current = intToNormalised<teensy_sample_t>(amp_in_current_b[i]);

        sample_t bell_output_sample = intToNormalised<teensy_sample_t>(bell_output_b[i]);

        static sample_t prev_synth_output_sample;


        //Apply volume level (simple linear scaling currently - could be improved)
        if (audio_shield_mode == AudioShieldMode::DEBUG || audio_shield_mode == AudioShieldMode::DISCONNECTED || audio_shield_mode == AudioShieldMode::HEADPHONE_OUTPUT || audio_shield_mode == AudioShieldMode::HP_OP_PIEZO_IP)
        {
            //Get User's volume setting
            float volume_level = usb_in.volume(); //0.0 - 1.0
            actuation_input_l *= volume_level;
            actuation_input_r *= volume_level;
        }


        //Cancel actuation signal from sensed signal
        TransducerFeedbackCancellation::UnprocessedSamples unprocessed;
        if (audio_shield_mode == AudioShieldMode::STANDALONE_SYNTH)
        {
            unprocessed.output_to_transducer = prev_synth_output_sample;
        }
        else
        {
            unprocessed.output_to_transducer = ( actuation_input_l + actuation_input_r ) / 2;
        }
        unprocessed.input_from_transducer = amp_in_current; //Current measurement from amp
        unprocessed.reference_input_loopback = amp_in_voltage; //Voltage measurement from amp
        TransducerFeedbackCancellation::ProcessedSamples processed = transducer_processing.process(unprocessed);

        sample_t usb_out_l, usb_out_r, amp_out, audio_shield_out_l, audio_shield_out_r;
        switch (audio_shield_mode)
        {
        case AudioShieldMode::ANALOG_ONLY:
            amp_out = processed.output_to_transducer;
            audio_shield_out_l = processed.input_feedback_removed;
            audio_shield_out_r = processed.input_feedback_removed;
            break;
        case AudioShieldMode::STANDALONE_SYNTH:
            audio_shield_out_l = prev_synth_output_sample;
            audio_shield_out_r = prev_synth_output_sample;
            prev_synth_output_sample = bell_output_sample;
            // prev_synth_output_sample = 0;
            //prev_synth_output_sample = kp_synth.process(processed.input_feedback_removed);
            amp_out = processed.output_to_transducer;
            break;
        case AudioShieldMode::DEBUG:
            usb_out_l = amp_in_current;
            usb_out_r = amp_in_voltage;//processed.input_feedback_removed;
            amp_out = actuation_input_l;
            break;
        case AudioShieldMode::DISCONNECTED:
            usb_out_l = processed.input_feedback_removed;
            usb_out_r = processed.input_feedback_removed;
            amp_out = processed.output_to_transducer;
            break;
        case AudioShieldMode::HEADPHONE_OUTPUT:
        case AudioShieldMode::HP_OP_PIEZO_IP:
            usb_out_l = processed.input_feedback_removed;
            usb_out_r = processed.input_feedback_removed;
            amp_out = processed.output_to_transducer;
            audio_shield_out_l = actuation_input_l;
            audio_shield_out_r = actuation_input_r;
            break;
        }
// Convert from normalised float back to int16 and add into output buffers
        bp_outL_audio_shield[i] = normalisedToInt<teensy_sample_t>(audio_shield_out_l) * dBToLin(headphone_level_db);
        bp_outR_audio_shield[i] = normalisedToInt<teensy_sample_t>(audio_shield_out_r) * dBToLin(headphone_level_db);
        bp_outL_i2s[i] = normalisedToInt<teensy_sample_t>(amp_out) * dBToLin(actuation_level_db);
        bp_outR_i2s[i] = normalisedToInt<teensy_sample_t>(amp_out) * dBToLin(actuation_level_db);
        bp_outL_usb[i] = normalisedToInt<teensy_sample_t>(usb_out_l);
        bp_outR_usb[i] = normalisedToInt<teensy_sample_t>(usb_out_r);
        bp_bell_input[i] =  normalisedToInt<teensy_sample_t>(processed.input_feedback_removed);

        force_sensing.process(processed.input_feedback_removed, processed.output_to_transducer);

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
    while(bell_input.playBuffer()){
        Serial.println("Play bell fail.");
    }

}

void setResonantFrequency(sample_t resonant_frequency_hz)
{
    current_cancellation_setup.resonant_frequency_hz = resonant_frequency_hz;
    force_sensing.setResonantFrequencyHz(resonant_frequency_hz);
    transducer_processing.setResonantFrequencyHz(resonant_frequency_hz);
    transducer_processing.setOscillatorFrequencyHz(resonant_frequency_hz);
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


} //Namespace AudioRouting