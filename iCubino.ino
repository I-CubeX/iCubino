
//
// iCubino
// by Infusion Systems Ltd. 
// 
// This Arduino sketch emulates some features of the firmware that runs on I-CubeX digitizers, 
// and allows you to use the Arduino, or rather "iCubino", with I-CubeX software that uses host mode: 
// Link, SensePlay, and various plugins. In all cases the I-CubeX Connect serial to MIDI mapping 
// software utility is required. Intended for use with the Uno and the I-CubeX Shield 
// (http://icubex.com/shield) that easily connects I-CubeX sensors to an Arduino. 
//
// See also http://icubex.com/icubino. 
//


#define RBL     128
#define MAXIN     6
#define MAXOUT   14
#define SX_MAP    0  // host mode
#define CV_MAP    1  // stand alone mode
#define RESET_PAUSE 1000
#define INTERVAL  10

byte rb[RBL];         // a buffer to hold incoming data
int rbt = 0;
int rbh = 0;
byte i = 0;

byte sxMsg6[6] = {240,125,0,0,0,247};
byte sxMsg7[7] = {240,125,0,0,0,0,247};
byte sxLen = 0;
byte sxCmd = 0;
byte resetMsg[5] = {240,125,0,35,247};
byte versionMsg[10] = {240,125,0,71,1,1,00,00,1,247}; // fw 0.1, hw 0.100, serial 0001 
byte streamHdr[4] = {240,125,0,0};
byte ctlMsg[3] = {176,0,0};

byte midiMap = SX_MAP;
byte mute = 0;
int samplingInterval = INTERVAL;
byte onChans = 0;
byte hiresChans = 0;
int sensVal = 0;
int mapVal = 0;
byte out_cmd = 0;

void setup() {
  //  Set MIDI baud rate:
  Serial.begin(115200);
 // while (!Serial) {
 //   ; // wait for serial port to connect. Needed for native USB port only
 // }

  Serial.write(resetMsg, 5);

  if (midiMap) {
    delay(RESET_PAUSE);
    Serial.write(resetMsg, 5);
  }

  for (i = 0; i < 14; i++)
    pinMode(i, OUTPUT);
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
    
    if ((sxLen == 5) && (sxCmd == 71)) { // DUMP VERSION command
      Serial.write(versionMsg, 10);
        
    } else if ((sxLen == 6) && (sxCmd == 1)) { // STREAM command
      if (rb[(rbt + RBL - 1) % RBL] & 64)
        onChans = onChans | (1 << (rb[(rbt + RBL - 1) % RBL] & 7));
      else
        onChans = onChans & (~(1 << (rb[(rbt + RBL - 1) % RBL] & 7)));
      sxMsg6[3] = 1;
      sxMsg6[4] = rb[(rbt + RBL - 1) % RBL];
      Serial.write(sxMsg6, 6);
        
   } else if ((sxLen == 6) && (sxCmd == 2)) { // RES command (not implemented)
      if (rb[(rbt + RBL - 1) % RBL] & 64)
        hiresChans = hiresChans | (1 << (rb[(rbt + RBL - 1) % RBL] & 7));
      else
        hiresChans = hiresChans & (~(1 << (rb[(rbt + RBL - 1) % RBL] & 7)));
      sxMsg6[3] = 2;
      sxMsg6[4] = rb[(rbt + RBL - 1) % RBL];
      Serial.write(sxMsg6, 6);
        
   } else if ((sxLen == 7) && (sxCmd == 3)) { // INTERVAL command
      samplingInterval = (rb[(rbt + RBL - 2) % RBL] << 7) | rb[(rbt + RBL - 1) % RBL];
      sxMsg7[3] = 3;
      sxMsg7[4] = samplingInterval >> 7;
      sxMsg7[5] = samplingInterval & 0x7F;
      Serial.write(sxMsg7, 7);
      
   } else if ((sxLen == 5) && (sxCmd == 13)) { // SEND INTERVAL command
      sxMsg7[3] = 3;
      sxMsg7[4] = samplingInterval >> 7;
      sxMsg7[5] = samplingInterval & 0x7F;
      Serial.write(sxMsg7, 7);
      
    } else if ((sxLen == 6) && (sxCmd == 48)) { // SET OUTPUT command
      out_cmd = rb[(rbt + RBL - 1) % RBL];
      if ((out_cmd & 0x0F) < 14)
        digitalWrite(out_cmd & 0x0F, out_cmd & 0x40);
      sxMsg6[3] = 48;
      sxMsg6[4] = out_cmd;
      Serial.write(sxMsg6, 6);
                
    } else if ((sxLen == 6) && (sxCmd == 50)) { // SET NUTE command
      mute = rb[(rbt + RBL - 1) % RBL];
        
    } else if ((sxLen == 6) && (sxCmd == 90)) { // SET MODE command
      midiMap = rb[(rbt + RBL - 1) % RBL];
      onChans = 0;
      hiresChans = 0;
      sxMsg6[3] = 91;
      sxMsg6[4] = midiMap;
      Serial.write(sxMsg6, 6);
        
    } else if ((sxLen == 5) && (sxCmd == 91)) { // DUMP MODE command
      sxMsg6[3] = 91;
      sxMsg6[4] = midiMap;
      Serial.write(sxMsg6, 6);
        
    } else if ((sxLen == 5) && (sxCmd == 34)) { // RESET command
      onChans = 0;
      hiresChans = 0;
      samplingInterval = INTERVAL;
      midiMap = SX_MAP;
      Serial.write(resetMsg, 5);
    }
    sxLen = 0;
  }
}

delay(samplingInterval);

if ((onChans) && (!mute)) {
  if (midiMap == SX_MAP) { // send sensor value as sysex MIDI message
  
    Serial.write(streamHdr, 4); // send STREAM message header
  
    for (i = 0; i < MAXIN; i++) {
  
      if (onChans & (1 << i)) {
      
        sensVal = analogRead(i);

        Serial.write(sensVal >> 3);
        if (hiresChans & (1 << i)) 
          Serial.write((sensVal & 7) << 2);
        
      }
    }
  
    Serial.write(247);
  
  } else if (midiMap == CV_MAP) { // send sensor value as control change channel voice MIDI message
  
    for (i = 0; i < MAXIN; i++) {
  
      if (onChans & (1 << i)) {
      
        sensVal = analogRead(i);
  
        mapVal = (sensVal * 128) / 1024;

        ctlMsg[1] = i;  // control change number
        ctlMsg[2] = mapVal; // control change value
        Serial.write(ctlMsg, 3);  // send control change message
                
      }
    }
  }
}

}
