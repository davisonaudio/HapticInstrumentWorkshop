let port;

let logging_strings = ['Haptic Instrument Design Study Serial Log'];
let logging_filename;

//Button objects
let connectBtn;
let sendDebugBtn;
let downloadLoggingBtn;

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

  connectBtn = createButton('Connect Serial');
  connectBtn.position(500, 30);
  connectBtn.mousePressed(connectBtnClick);

  sendDebugBtn = createButton('Enter Debug Mode');
  sendDebugBtn.position(500, 60);
  sendDebugBtn.mousePressed(sendDebugBtnClick);
  
  downloadLoggingBtn = createButton('Download Logging txt');
  downloadLoggingBtn.position(500, 90);
  downloadLoggingBtn.mousePressed(downloadLogging);
}

function draw() {
  // this makes received text scroll up
  

  // reads in complete lines and prints them at the
  // bottom of the canvas
  let str = port.readUntil("\n");
  if (str.length > 0) {
    logPrint("Teensy: " + str);
  }

  // changes button label based on connection status
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

function downloadLogging(){
  saveStrings(logging_strings, logging_filename);
}

function logPrint(log_str) {
  copy(0, 0, width, height, 0, -20, width, height);
  text(log_str, 10, height-20);
  append(logging_strings,log_str);
}