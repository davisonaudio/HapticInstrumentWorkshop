/*

TransducerFeedbackCancellation.h

Author: Matt Davison
Date: 05/03/2024

*/

#pragma once

#include "au_Biquad.h"
#include "au_Onepole.h"
#include "au_ToneGenerator.h"


class TransducerFeedbackCancellation
{
public:
    enum class AmplifierType
    {
        CURRENT_DRIVE,
        VOLTAGE_DRIVE
    };

    struct Setup
    {
        sample_t resonant_frequency_hz;
        sample_t resonance_peak_gain_db;
        sample_t resonance_q;
        sample_t resonance_tone_level_db;
        sample_t inductance_filter_coefficient;
        sample_t transducer_input_wideband_gain_db;
        sample_t sample_rate_hz;
        AmplifierType amplifier_type;
        bool lowpass_transducer_io = true;
    };

    void setResonantFrequencyHz(sample_t resonant_frequency_hz);
    void setOscillatorFrequencyHz(sample_t oscillator_frequency_hz);
    void setResonancePeakGainDb(sample_t resonance_peak_gain_db);
    void setResonanceQ(sample_t resonance_q);
    void setResonanceToneLevelDb(sample_t resonance_tone_level_db);
    void setInductanceFilterCoefficient(sample_t inductance_filter_coefficient);
    void setTransducerInputWidebandGainDb(sample_t transducer_input_wideband_gain_db);
    void setAdmittanceFilter();

    struct UnprocessedSamples
    {
        sample_t output_to_transducer;
        sample_t input_from_transducer;
        sample_t reference_input_loopback;
    };

    struct ProcessedSamples
    {
        sample_t input_feedback_removed;
        sample_t output_to_transducer;
        sample_t modelled_signal;
        sample_t transducer_return_with_gain_applied;
    };

    struct AdmittanceFilter
    {
        int order;
        std::vector<Biquad> biquads;
    };


    void setup(Setup setup_parameters);

    ProcessedSamples process(UnprocessedSamples unprocessed);

private:

    Biquad m_resonance_filter;
    Onepole m_series_inductance_filter;
    ToneGenerator m_resonance_tone;
    sample_t m_transducer_input_wideband_gain_lin;

    Biquad m_output_to_transducer_lowpass;
    Biquad m_input_from_transducer_lowpass;
    bool m_lowpass_filters_enabled;
    AmplifierType amplifier_type;
    AdmittanceFilter admittance_filter;

    sample_t applyTransducerModelFilter(sample_t input_sample);
};


















/*
* Implementation of methods
*/


void TransducerFeedbackCancellation::setResonantFrequencyHz(sample_t resonant_frequency_hz)
{
    m_resonance_filter.setCutoff(resonant_frequency_hz);
}

void TransducerFeedbackCancellation::setOscillatorFrequencyHz(sample_t oscillator_frequency_hz)
{
    m_resonance_tone.setFrequency(oscillator_frequency_hz);
}

void TransducerFeedbackCancellation::setResonancePeakGainDb(sample_t resonance_peak_gain_db)
{
    m_resonance_filter.setFilterGain(resonance_peak_gain_db);  
}

void TransducerFeedbackCancellation::setResonanceQ(sample_t resonance_q)
{
    m_resonance_filter.setQ(resonance_q);
}

void TransducerFeedbackCancellation::setResonanceToneLevelDb(sample_t resonance_tone_level_db)
{
    m_resonance_tone.setLeveldB(resonance_tone_level_db);
}
void TransducerFeedbackCancellation::setInductanceFilterCoefficient(sample_t inductance_filter_coefficient)\
{
    m_series_inductance_filter.setB1(inductance_filter_coefficient);
}

void TransducerFeedbackCancellation::setTransducerInputWidebandGainDb(sample_t transducer_input_wideband_gain_db)
{
    m_transducer_input_wideband_gain_lin = dBToLin(transducer_input_wideband_gain_db);
}

void TransducerFeedbackCancellation::setAdmittanceFilter()
{
    //Stacked biquads
    admittance_filter.order = 3;
    Biquad temp;
    Biquad::Coefficients temp_coefficients;
    temp_coefficients.a0 = 0.29781613;
    temp_coefficients.a1 = 0.00272268;
    temp_coefficients.a2 = 0;
    temp_coefficients.b1 = 0;
    temp_coefficients.b2 = 0;
    temp.setCoefficients(temp_coefficients);
    admittance_filter.biquads.push_back(temp);

    temp_coefficients.a0 = 1;
    temp_coefficients.a1 = -0.00933585;
    temp_coefficients.a2 = 0;
    temp_coefficients.b1 = -0.00242049;
    temp_coefficients.b2 = -0.00095691;
    temp.setCoefficients(temp_coefficients);
    admittance_filter.biquads.push_back(temp);

    temp_coefficients.a0 = 1;
    temp_coefficients.a1 = -1.99746549;
    temp_coefficients.a2 = 0.99830338;
    temp_coefficients.b1 = -1.96034539;
    temp_coefficients.b2 = 0.9612116;
    temp.setCoefficients(temp_coefficients);
    admittance_filter.biquads.push_back(temp);
}


void TransducerFeedbackCancellation::setup(Setup setup_parameters)
{
    //Setup of transducer resonance modelling filter, using peak biquad
    Biquad::FilterSetup resonance_filter_setup;
    resonance_filter_setup.cutoff_freq_hz = setup_parameters.resonant_frequency_hz;
    resonance_filter_setup.filter_gain_db = setup_parameters.resonance_peak_gain_db;
    resonance_filter_setup.quality_factor = setup_parameters.resonance_q;
    resonance_filter_setup.sample_rate_hz = setup_parameters.sample_rate_hz;
    resonance_filter_setup.filter_type = Biquad::FilterType::PEAK;

    m_resonance_filter.setup(resonance_filter_setup);

    //Setup of series inductance modelling filter (causes rise in impedance at HF) using onepole filter
    m_series_inductance_filter.setB1(setup_parameters.inductance_filter_coefficient);

    m_transducer_input_wideband_gain_lin = dBToLin(setup_parameters.transducer_input_wideband_gain_db);

    //Setup of tone generator
    m_resonance_tone.setFrequency(setup_parameters.resonant_frequency_hz);
    m_resonance_tone.setSampleRate(setup_parameters.sample_rate_hz);
    m_resonance_tone.setLeveldB(setup_parameters.resonance_tone_level_db);


    //Setup lowpass filters
    m_lowpass_filters_enabled = setup_parameters.lowpass_transducer_io;
    Biquad::FilterSetup lowpass_setup;
    lowpass_setup.cutoff_freq_hz = 500.0;
    lowpass_setup.filter_gain_db = 0.0;
    lowpass_setup.quality_factor = 0.701;
    lowpass_setup.sample_rate_hz = setup_parameters.sample_rate_hz;
    lowpass_setup.filter_type = Biquad::FilterType::LOWPASS;

    m_output_to_transducer_lowpass.setup(lowpass_setup);
    m_input_from_transducer_lowpass.setup(lowpass_setup);

    amplifier_type = setup_parameters.amplifier_type;

    if(amplifier_type == AmplifierType::VOLTAGE_DRIVE){
        setAdmittanceFilter();
    }
}

TransducerFeedbackCancellation::ProcessedSamples TransducerFeedbackCancellation::process(UnprocessedSamples unprocessed)
{
    ProcessedSamples processed;

    if(amplifier_type == AmplifierType::CURRENT_DRIVE){
        //Process input from transducer
        processed.modelled_signal = applyTransducerModelFilter(unprocessed.reference_input_loopback);

        processed.transducer_return_with_gain_applied = m_transducer_input_wideband_gain_lin * unprocessed.input_from_transducer;

        processed.input_feedback_removed = processed.transducer_return_with_gain_applied - processed.modelled_signal;

        //Process output to transducer
        processed.output_to_transducer = unprocessed.output_to_transducer + m_resonance_tone.process();

        if (m_lowpass_filters_enabled)
        { //Filter to signal to/from transducer
            processed.output_to_transducer = m_output_to_transducer_lowpass.process(processed.output_to_transducer);
            processed.input_feedback_removed = m_input_from_transducer_lowpass.process(processed.input_feedback_removed);
        }
    }

    else if(amplifier_type == AmplifierType::VOLTAGE_DRIVE){
        processed.modelled_signal = applyTransducerModelFilter(unprocessed.reference_input_loopback);

        processed.transducer_return_with_gain_applied = m_transducer_input_wideband_gain_lin * unprocessed.input_from_transducer;
        processed.input_feedback_removed = processed.transducer_return_with_gain_applied - processed.modelled_signal;
        processed.output_to_transducer = unprocessed.output_to_transducer;
        if (m_lowpass_filters_enabled)
        { //Filter to signal to/from transducer
            processed.output_to_transducer = m_output_to_transducer_lowpass.process(processed.output_to_transducer);
            processed.input_feedback_removed = m_input_from_transducer_lowpass.process(processed.input_feedback_removed);
        }
    }

    return processed;
}

sample_t TransducerFeedbackCancellation::applyTransducerModelFilter(sample_t input_sample)
{
     sample_t output;
    if(amplifier_type == AmplifierType::CURRENT_DRIVE){
        output = m_resonance_filter.process(input_sample);

        //output = m_series_inductance_filter.process(output);
    }
    else{
        output = input_sample;
        for(int i = 0; i < admittance_filter.order; i++){
            output = admittance_filter.biquads[i].process(output);
        }
    }
    return output;
}

