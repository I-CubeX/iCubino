
#define RBL     128
#define MAXCHAN   8
#define SX_MAP    0  // host mode
#define CV_MAP    1  // stand alone mode
#define RESET_PAUSE 1000

byte rb[RBL];         // a buffer to hold incoming data
int rbt = 0;
int rbh = 0;
byte i = 0;

byte sxMsg6[6] = {240,125,0,0,0,247};
byte sxMsg7[7] = {240,125,0,0,0,0,247};
byte sxLen = 0;
byte sxCmd = 0;
byte resetMsg[5] = {240,125,0,35,247};
byte versionMsg[10] = {240,125,0,71,10,10,00,01,23,247};

byte midiMap = 0;
int samplingInterval = 10;
byte onChans = 0;
int sensVal = 0;
int mapVal = 0;

void setup() {
  //  Set MIDI baud rate:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  for (i = 0; i < 5; i++) 
    Serial.write(resetMsg[i]);

  if (midiMap) {

    delay(RESET_PAUSE);

    for (i = 0; i < 5; i++) 
      Serial.write(resetMsg[i]);
  }
}

void loop() {

while (Serial.available() > 0) {
    // get incoming byte:
    rbh = (rbh + 1) % RBL;
    rb[rbh] = Serial.read();
}    

while (rbt != rbh) {
  
  rbt = (rbt + 1) % RBL;
  
  if (rb[rbt] == 240)
    sxLen++;
  else if (sxLen == 3) {
     sxLen++;
     sxCmd = rb[rbt];
  }
  else if ((sxLen) && (rb[rbt] != 247))
    sxLen++;
  else if (rb[rbt] == 247) {
    sxLen++;
        
    if ((sxLen == 5) && (sxCmd == 71)) { // VERSION command
      for (i = 0; i < 10; i++) 
        Serial.write(versionMsg[i]);
        
    } else if ((sxLen == 6) && (sxCmd == 1)) { // STREAM command
      if (rb[(rbt + RBL - 1) % RBL] & 64)
        onChans = onChans | (1 << (rb[(rbt + RBL - 1) % RBL] & 7));
      else
        onChans = onChans & (~(1 << (rb[(rbt + RBL - 1) % RBL] & 7)));
      sxMsg6[3] = 1;
      sxMsg6[4] = rb[(rbt + RBL - 1) % RBL];
      for (i = 0; i < 6; i++) 
        Serial.write(sxMsg6[i]);
        
   } else if ((sxLen == 6) && (sxCmd == 2)) { // RES command (not implemented)
      sxMsg6[3] = 2;
      sxMsg6[4] = rb[(rbt + RBL - 1) % RBL];
      for (i = 0; i < 6; i++) 
        Serial.write(sxMsg6[i]);
        
   } else if ((sxLen == 7) && (sxCmd == 3)) { // INTERVAL command
      samplingInterval = (rb[(rbt + RBL - 2) % RBL] << 7) | rb[(rbt + RBL - 1) % RBL];
      sxMsg7[3] = 3;
      sxMsg7[4] = samplingInterval >> 7;
      sxMsg7[5] = samplingInterval & 0x7F;
      for (i = 0; i < 7; i++) 
        Serial.write(sxMsg7[i]);
      
    } else if ((sxLen == 6) && (sxCmd == 90)) { // MODE command
      midiMap = rb[(rbt + RBL - 1) % RBL];
      sxMsg6[3] = 91;
      sxMsg6[4] = midiMap;
      for (i = 0; i < 6; i++) 
        Serial.write(sxMsg6[i]);
        
    } else if ((sxLen == 5) && (sxCmd == 34)) { // RESET command
      for (i = 0; i < 5; i++) 
        Serial.write(resetMsg[i]);
    }
    sxLen = 0;
  }
}

delay(samplingInterval);

if (onChans) {
  if (midiMap == SX_MAP) { // send sensor value as sysex MIDI message
  
    Serial.write(240);
    Serial.write(125);
    Serial.write(0);
    Serial.write(0); // STREAM message
  
    for (i = 0; i < MAXCHAN; i++) {
  
      if (onChans & (1 << i)) {
      
        sensVal = analogRead(i);
  
        Serial.write(sensVal >> 3);
        Serial.write((sensVal & 7) << 2);
        
      }
    }
  
    Serial.write(247);
  
  } else if (midiMap == CV_MAP) { // send sensor value as control change channel voice MIDI message
  
    for (i = 0; i < MAXCHAN; i++) {
  
      if (onChans & (1 << i)) {
      
        sensVal = analogRead(i);
  
        mapVal = (sensVal * 128) / 1024;
        
        Serial.write(176);  // control change message
        Serial.write(i);    // control change number
        Serial.write(mapVal);
        
      }
    }
  }
}

}
