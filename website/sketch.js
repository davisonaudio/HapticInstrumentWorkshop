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

let midiIn;
let midiOut;
let midiOutChannel;

let debug_mode = false;

function setup() {
  createCanvas(700, 500);
  background(220);
  
  textSize(12);

  setupLoggingFile();

  port = createSerial();

  //Open a previously used port if available (if not then dialog will appear in browser)
  let usedPorts = usedSerialPorts();
  if (usedPorts.length > 0) {
    port.open(usedPorts[0], 115200);
  }

  text("Serial Controls", 500, 20);
  connectBtn = createButton('Connect Serial');
  connectBtn.position(500, 30);
  connectBtn.mousePressed(connectBtnClick);

  sendDebugBtn = createButton('Enter Debug Mode');
  sendDebugBtn.position(500, 60);
  sendDebugBtn.mousePressed(sendDebugBtnClick);
  
  downloadLoggingBtn = createButton('Download Logging txt');
  downloadLoggingBtn.position(500, 90);
  downloadLoggingBtn.mousePressed(downloadLogging);
  
  
  
  text("MIDI Controls", 500, 140);
  midiInPortDropdown = createSelect();
  midiInPortDropdown.position(500, 150);
  midiInPortDropdown.option("None");
  
  midiInConnectBtn = createButton("Connect MIDI In");
  midiInConnectBtn.position(500, 180);
  midiInConnectBtn.mousePressed(connectMidiIn);
  
  midiOutPortDropdown = createSelect();
  midiOutPortDropdown.position(500, 210);
  midiOutPortDropdown.option("None");
  
  midiInConnectBtn = createButton("Connect MIDI Out");
  midiInConnectBtn.position(500, 240);
  midiInConnectBtn.mousePressed(connectMidiOut);
  
  saveToEepromButton = createButton("Save Parameters to EEPROM");
  saveToEepromButton.position(500, 270);
  saveToEepromButton.mousePressed(saveToEeprom);
  
  calibrateDampedButton = createButton("Calibrate Damped Level");
  calibrateDampedButton.position(500, 300);
  calibrateDampedButton.mousePressed(calibrateDamped);
  
  calibrateUndampedButton = createButton("Calibrate Undamped Level");
  calibrateUndampedButton.position(500, 330);
  calibrateUndampedButton.mousePressed(calibrateUndamped);
  
  
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
  copy(0, 0, width, height, 0, -20, width, height);
  text(log_str, 10, height-20);
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