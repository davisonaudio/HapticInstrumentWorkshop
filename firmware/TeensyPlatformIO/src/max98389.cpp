#include "max98389.h"
#include <i2c_device.h>

void max98389::begin(uint32_t frequency){
    master.begin(frequency);
}

bool max98389::configure(){
    uint8_t revisionId;
    // Check the revision ID and check for I2C errors
    if (amp.read(revision_id_register, &revisionId, false)) {
        if ( revisionId != expected_revision_id) {
            Serial.printf("ERROR: Manufacturer ID is 0x%X. Expected 0x%X.\n", revisionId, expected_revision_id);
            return false;
        }
    } else {
        report_error("ERROR: Failed to send manufacturer id register value");
        return false;
    }

    if(!amp.write(pcm_mode_register, (uint8_t) 0b11000000, false)){
        report_error("ERROR: Failed to write PCM Mode.");
        return false;
    }
    if(!amp.write(pcm_clock_register, (uint8_t) 0b00000100, false)){
        report_error("ERROR: Failed to write PCM Clock.");
        return false;
    }
    if(!amp.write(pcm_sample_rate_register, (uint8_t) 0x47, false)){
        report_error("ERROR: Failed to write PCM Sample Rate.");
        return false;
    }
    if(!amp.write(pcm_vmon_slots_register, (uint8_t) 0x00, false)){
        report_error("ERROR: Failed to write PCM V Monitor Slots.");
        return false;
    }
    if(!amp.write(pcm_imon_slots_register, (uint8_t) 0x01, false)){
        report_error("ERROR: Failed to write PCM I Monitor Slots.");
        return false;
    }
    if(!amp.write(pcm_tx_source_en_register, (uint8_t) 0x03, false)){
        report_error("ERROR: Failed to write Tx Source Enable.");
        return false;
    }
    if(!amp.write(pcm_rx_en_register, (uint8_t) 0x01, false)){
        report_error("ERROR: Failed to write PCM Rx Enable.");
        return false;
    }
    if(!amp.write(pcm_tx_en_register, (uint8_t) 0x03, false)){
        report_error("ERROR: Failed to write PCM Tx Enable.");
        return false;
    }

    if(!amp.write(auto_recovery_register, (uint8_t) 0x01, false)){
        report_error("ERROR: Failed to write to auto recovery.");
        return false;
    }

    if(!amp.write(amp_en_register, (uint8_t) 0x01, false)){
        report_error("ERROR: Failed to write Amp Enable.");
        return false;
    }
    if(!amp.write(iv_data_en_register, (uint8_t) 0x03, false)){
        report_error("ERROR: Failed to write IV Data Enable.");
        return false;
    }

    for(uint16_t i = 0; i < 7; i++){
        amp.write((uint16_t)(tx_hi_z_control1+i), (uint8_t) 0x00, false);
    }

    if(!amp.write(global_en_register, (uint8_t) 0x01, false)){
        report_error("ERROR: Failed to Global Enable.");
        return false;
    }

    Serial.println("Configured sensor successfully.");
    return true;
}

void max98389::report_error(const char* message) {
    Serial.print(message);
    Serial.print(" Error: ");
    Serial.println((int)master.error());
}