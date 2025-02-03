const saveToEepromPgmChange = 0;
const calibrateDampedPgmChange = 1;
const calibrateUndampedPgmChange = 2;

function sendSaveToEepromMidi(midiChannel)
{
  midiChannel.sendProgramChange(saveToEepromPgmChange);
}

function calibrateDampedMidi(midiChannel)
{
  midiChannel.sendProgramChange(calibrateDampedPgmChange);
}

function calibrateUndampedMidi(midiChannel)
{
  midiChannel.sendProgramChange(calibrateUndampedPgmChange);
}
