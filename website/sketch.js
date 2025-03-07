let port;

let logging_strings = ['Haptic Instrument Design Study Serial Log'];
let logging_filename;

//Button objects
let connectBtn;
let sendDebugBtn;
let downloadLoggingBtn;

let midiInPortDropdown;
let midiOutPortDropdown;
let midiInConnectBtn;
let midiOutConnectBtn;

let saveToEepromButton;
let calibrateDampedButton;
let calibrateUndampedButton;
let resetToDefaultParametersButton;

let serialInputTextbox;
let serialInputButton;

let resonantFrequencySlider

let midiIn;
let midiOut;
let midiOutChannel;

let debug_mode = false;



function setup() {
  createCanvas(1000, 500);
  background(220);
  
  textSize(12);

  setupLoggingFile();

  port = createSerial();

  //Open a previously used port if available (if not then dialog will appear in browser)
  let usedPorts = usedSerialPorts();
  if (usedPorts.length > 0) {
    port.open(usedPorts[0], 115200);
  }

  let x_pos_of_column = 800;
  let serialText = text("Serial Controls", x_pos_of_column, 20);
  connectBtn = createButton('Connect Serial');
  connectBtn.position(x_pos_of_column, 30);
  connectBtn.mousePressed(connectBtnClick);

  sendDebugBtn = createButton('Enter Debug Mode');
  sendDebugBtn.position(x_pos_of_column, connectBtn.position().y + 30);
  sendDebugBtn.mousePressed(sendDebugBtnClick);
  
  downloadLoggingBtn = createButton('Download Logging txt');
  downloadLoggingBtn.position(x_pos_of_column, sendDebugBtn.position().y + 30);
  downloadLoggingBtn.mousePressed(downloadLogging);
  
  
  
  text("MIDI Controls", x_pos_of_column, downloadLoggingBtn.position().y + 30 + 120);
  midiInPortDropdown = createSelect();
  midiInPortDropdown.position(x_pos_of_column, downloadLoggingBtn.position().y + 160);
  midiInPortDropdown.option("None");
  
  midiInConnectBtn = createButton("Connect MIDI In");
  midiInConnectBtn.position(x_pos_of_column, midiInPortDropdown.position().y + 25);
  midiInConnectBtn.mousePressed(connectMidiIn);
  
  midiOutPortDropdown = createSelect();
  midiOutPortDropdown.position(x_pos_of_column, midiInConnectBtn.position().y + 35);
  midiOutPortDropdown.option("None");
  
  midiInConnectBtn = createButton("Connect MIDI Out");
  midiInConnectBtn.position(x_pos_of_column, midiOutPortDropdown.position().y + 25);
  midiInConnectBtn.mousePressed(connectMidiOut);
  
  saveToEepromButton = createButton("Save Parameters to EEPROM");
  saveToEepromButton.position(x_pos_of_column, midiInConnectBtn.position().y + 30);
  saveToEepromButton.mousePressed(saveToEeprom);
  
  calibrateDampedButton = createButton("Calibrate Damped Level");
  calibrateDampedButton.position(x_pos_of_column, saveToEepromButton.position().y + 30);
  calibrateDampedButton.mousePressed(calibrateDamped);
  
  calibrateUndampedButton = createButton("Calibrate Undamped Level");
  calibrateUndampedButton.position(x_pos_of_column, calibrateDampedButton.position().y + 30);
  calibrateUndampedButton.mousePressed(calibrateUndamped);

  resetToDefaultParametersButton = createButton("Reset Parameters to Defaults");
  resetToDefaultParametersButton.position(x_pos_of_column, calibrateUndampedButton.position().y + 30);
  resetToDefaultParametersButton.mousePressed(resetParameters);

  serialInputTextbox = createInput("");
  serialInputTextbox.position(10, 470);
  serialInputTextbox.size(300);

  serialInputButton = createButton("Send Serial");
  serialInputButton.position(320, 470);
  serialInputButton.mousePressed(sendSerialText);
  
  resonantFrequencySlider = new SliderWithTextbox(-1, 1, 0, 0.01, sendResonantFreq);
  
  WebMidi.enable(onMidiEnabled);
}

function draw() {
  

  // read in complete lines
  let str = port.readUntil("\n");
  if (str.length > 0) {
    logPrint("Teensy: " + str);
  }

  // change button label based on connection status
  if (!port.opened()) {
    connectBtn.html('Connect Serial');
  } else {
    connectBtn.html('Disconnect Serial');
  }
}

function connectBtnClick() {
  if (!port.opened()) {
    port.open(115200);
    logPrint("p5: Opened serial port ")
  } else {
    port.close();
  }
}

function sendDebugBtnClick() {
  if (debug_mode){
    port.write("normal\n");
    debug_mode = false;
    sendDebugBtn.html('Enter Debug Mode');
  }
  else{
    port.write("debug\n");
    debug_mode = true;
    sendDebugBtn.html('Enter Normal Mode');
  }
}

function setupLoggingFile() {
  let date_string = "Date: " + join([str(day()),str(month()),str(year())],'/');
  let time_string = "Time: " + join([str(hour()),str(minute()),str(second())],':');
  append(logging_strings, date_string);
  append(logging_strings, time_string);
  
  logging_filename = "teensy_logging_" + str(year()) + '_' + str(month()) + '_' + str(day()) + '_' + str(hour()) + str(minute()) + str(second()) + ".txt";
}

function downloadLogging() {
  saveStrings(logging_strings, logging_filename);
}

function logPrint(log_str) {
  copy(0, 0, width - 200, height, 0, -20, width - 200, height);
  text(log_str, 10, height-40);
  append(logging_strings,log_str);
}

function onMidiEnabled() {
  for (let i = 0; i < WebMidi.inputs.length; i++) {
    midiInPortDropdown.option(WebMidi.inputs[i].name);
  }
  
  for (let i = 0; i < WebMidi.outputs.length; i++) {
    midiOutPortDropdown.option(WebMidi.outputs[i].name);
  }
  
}

function connectMidiIn() {
  midiIn = WebMidi.getInputByName(midiInPortDropdown.selected());
}

function connectMidiOut() {
  midiOut = WebMidi.getOutputByName(midiOutPortDropdown.selected());
  midiOutChannel = midiOut.channels[1];
}

function saveToEeprom() {
  sendSaveToEepromMidi(midiOutChannel);
}

function calibrateDamped() {
  calibrateDampedMidi(midiOutChannel);
}

function calibrateUndamped() {
  calibrateUndampedMidi(midiOutChannel);
}

function resetParameters() {
  resetParametersMidi(midiOutChannel);
}

function sendResonantFreq() {
  
}

function sendSerialText(){
  port.write(serialInputTextbox.value() + "\n");
  serialInputTextbox.value("");
}