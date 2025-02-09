const saveToEepromPgmChange = 0;
const resetToDefaultParameters = 1;
const calibrateDampedPgmChange = 2;
const calibrateUndampedPgmChange = 3;

function sendSaveToEepromMidi(midiChannel)
{
  midiChannel.sendProgramChange(saveToEepromPgmChange);
}

function resetParametersMidi(midiChannel)
{
  midiChannel.sendProgramChange(resetToDefaultParameters);
}

function calibrateDampedMidi(midiChannel)
{
  midiChannel.sendProgramChange(calibrateDampedPgmChange);
}

function calibrateUndampedMidi(midiChannel)
{
  midiChannel.sendProgramChange(calibrateUndampedPgmChange);
}

