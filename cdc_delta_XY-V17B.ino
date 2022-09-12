/////////////////////////////////////////////////
// CDC Emulator + mp3 Player for Audi Delta CC //
// Atmega168 Pro Mini with player XY-V17B v.5s //
/////////////////////////////////////////////////

#include <Wire.h>
#include "Arduino.h"
#include "SoftwareSerial.h"
#include <EEPROM.h>

#define HU_START          (uint16_t)0xA121
#define HU_STOP           (uint16_t)0x21A1
#define HU_LEFT           (uint16_t)0x0181
#define HU_RIGHT          (uint16_t)0x0282
#define HU_DOWN           (uint16_t)0x0383
#define HU_UP             (uint16_t)0x0484
#define HU_SCAN           (uint16_t)0x8707
#define HU_RND            (uint16_t)0xA323
#define HU_1              (uint16_t)0x9901
#define HU_2              (uint16_t)0x9902
#define HU_3              (uint16_t)0x9903
#define HU_4              (uint16_t)0x9904
#define HU_5              (uint16_t)0x9905
#define HU_6              (uint16_t)0x9906

SoftwareSerial MP3(7, 8);

uint32_t timer = millis();
uint8_t i;
int16_t fi;
int16_t ff = 0;
int16_t cf;
int16_t pf;
uint8_t sc = 0;
uint8_t tv = 0;
uint8_t cd = 1;
uint8_t tr = 1;
uint8_t cd1 = 1;
uint8_t tr1 = 1;
uint8_t cdc = 0;
uint8_t stp = 0;
uint8_t info = 0;
uint8_t volume = 25;
uint8_t isPlay = 0;
boolean isScan = false;
boolean isRandom = false;
boolean isPlaying = false;
uint8_t play[] =  {0xAA, 0x02, 0x00, 0xAC};
uint8_t pause[] = {0xAA, 0x03, 0x00, 0xAD};
uint8_t previous[] = {0xAA, 0x05, 0x00, 0xAF}; 
uint8_t next[] = {0xAA, 0x06, 0x00, 0xB0};
uint8_t selectSong[] = {0xAA, 0x1F, 0x02, 0x00, 0x0A, 0xD5}; 
uint8_t vol[] = {0xAA, 0x13, 0x01, 0x00, 0xBE}; 

void setup() {
  wire();
  MP3.begin(9600);
  pp(350);
  uint8_t numberSongs[] = {0xAA, 0x0C, 0x00, 0xB6};
  uint8_t folderSongs[] = {0xAA, 0x12, 0x00, 0xBC};
  uint8_t noTracks[6] = {0};  
  uint8_t indx = 0; 
  MP3.write(numberSongs, 4);
  MP3.flush();
while (!MP3.available()) pp(20);
while (MP3.available()) {
    noTracks[indx] = MP3.read();
    indx++;
    pp(10);
  }
  fi = 256*noTracks[3] + noTracks[4];
  EEPROM.get(251, ff);

if (ff != fi) {
cd1 = 0; tr1 = 255;
report();  
uint8_t fe = 0;
i = 0;
ff = 1;

while (ff < fi) {
  i++;
  selectSong[3] = ff>>8;
  selectSong[4] = ff;
  selectSong[5] = selectSong[3] + selectSong[4] + 0xCB;
  MP3.write(selectSong, 6);
  pp(10);
  noTracks[6] = {0};
  indx = 0; 
  MP3.write(folderSongs, 4);
  MP3.flush();
  pp(20);
  while (!MP3.available()) pp(20);
  while (MP3.available()) {
    noTracks[indx] = MP3.read();
    indx++;
    pp(5);
    }
  fe = noTracks[4];
  ff = ff + fe;
  EEPROM.update(i, fe);
  cd1 = i; tr1 = 255;
  while (cd1 > 15) {cd1 -= 15;}
  report();
  }
EEPROM.update(0, i);
EEPROM.put(251, fi);
}

  if (EEPROM.read(255) > 30 || EEPROM.read(255) < 0) EEPROM.update(255, 25);
  if (EEPROM.read(256) > 1) EEPROM.update(256, 0);
  if (EEPROM.read(257) > 1) EEPROM.update(257, 0);
  
  volume = EEPROM.read(255);
  vol[3] = volume;
  vol[4] = 0xBE + vol[3];
  MP3.write(vol, 5);
  isScan = EEPROM.read(256);
  isRandom = EEPROM.read(257);
   
  EEPROM.get(253, cf);
  if (cf > fi) cf = 1;
  selectSong[3] = cf>>8;
  selectSong[4] = cf;
  selectSong[5] = selectSong[3] + selectSong[4] + 0xCB;
  MP3.write(selectSong, 6);
  track(); 
  report();
  stat();
  if (cdc == 16) cdc = 0;
}

void loop() {
  getMP3Status();
  if ((isPlaying) && isPlay == 0) {
    if (isRandom) {
        pf = cf;
        cf = random(fi) + 1;
        playSong();
       } else {
         MP3.write(next, 4);
         cf++;
         if (cf > fi) cf = 1; 
       } 
      track();
      report();
  }

    if (millis() - timer >= 1000) {
      if (isPlaying) {
        timer = millis(); 
        if (isScan) {
          sc++;
          if (sc == 8) {
            tr1 = 238;
            report();
          }
        }
      }
         
         if (tv > 0) {
         tv--;
          if (tv == 1) {
            EEPROM.update(255, volume);
            track();
            report();
          }
      }

         if (info > 0) {
         info--;
          if (info == 5 && cdc == 10) {
          tr1 = 221;
          report();
          pp(400);
          uint8_t tr2 = EEPROM.read(cd);
          if (tr2 > 99) tr2 = 99;
          tr1 = tr2/10*16 + tr2%10; 
          report();  
          }
          if (info == 3 && cdc == 10) {
          cdc = 0;  
          tr1 = 236;
          report();
          pp(400);
          uint8_t tr2 = EEPROM.read(0);
          if (tr2 > 99) tr2 = 99;
          tr1 = tr2/10*16 + tr2%10; 
          report();  
          }
        
          if (info == 1) {
            track();
            report();
          }
        }
    }

    if (sc >= 10) {
      sc = 0;
      if (isRandom) {
        pf = cf;
        cf = random(fi) + 1;
        playSong();
      } else {
        MP3.write(next, 4);
        cf++;
        if (cf > fi) cf = 1;
      }
      track();
      report();
    }  
  
  
    
  if (cdc != 0) {
   if (cdc == 15) {
       isPlaying = true;
       MP3.write(play, 4);
       cdc = 0;
       track();
       report();
    }
    
    if (cdc == 16) {
        isPlaying = false;
        MP3.write(pause, 4); 
        EEPROM.put(253, cf);
        EEPROM.update(256, isScan);
        EEPROM.update(257, isRandom);
        sc = 0; 
        cdc = 0;
    }

    if (cdc == 14) {
        cdc = 0;
        if (isPlaying) {
          if (isRandom) {
            pf = cf;
            cf = random(fi) + 1;
            playSong();
          } else {
            MP3.write(next, 4); 
            cf++;
            if (cf > fi) cf = 1;
          }
          track();
          report();
          if (isScan) sc = 0;
        }
    }

    if (cdc == 13) {
        cdc = 0;
        if (isPlaying) {
          if (isRandom) {
            cf = pf;
            playSong();
          } else {
            MP3.write(previous, 4);  
            cf--;
            if (cf < 1) cf = fi;
          }
          track();
          report();
          if (isScan) {
            sc = 0;
            isScan = false;
            stat();
          }
       }
    }  
    
    if (cdc == 11) {
        cdc = 0;
        if (volume < 29) {
              volume++;
              vol[3] = volume;
              vol[4] = 0xBE + vol[3];
              MP3.write(vol, 5);
              tv = 6;
              cd1 = 0;
              tr1 = volume/10*16 + volume%10;
              report();
        }     
    }
    
    if (cdc == 12) {
        cdc = 0;
        if (volume > 0) {
              volume--;
              vol[3] = volume;
              vol[4] = 0xBE + vol[3];
              MP3.write(vol, 5);
              tv = 6;
              cd1 = 0;
              tr1 = volume/10*16 + volume%10;
              report();
        }   
    }
     
    if (cdc == 7) {
        cdc = 0;
        isScan = !isScan;
        sc = 0;
        stat();
    }
    
    if (cdc == 8) {
        cdc = 0;
        isRandom = !isRandom;
        stat();
    }    
        
    if (cdc == 1) {
        cdc = 9;
        if (cd == 1) {
          cd = EEPROM.read(0);
        } else cd--;
        
        cf = 1;
        for (i = 1; i < cd; i++) {
          cf = cf + EEPROM.read(i);
        } 
        playSong();
        if (isScan) sc = 0;
        track();
    }

    if (cdc == 2) {
        cdc = 9;
        if (cd < EEPROM.read(0)) cd++;
        else cd = 1;
        
        cf = 1;
        for (i = 1; i < cd; i++) {
          cf = cf + EEPROM.read(i);
        }
        playSong();
        if (isScan) sc = 0;
        track();
    }

    if (cdc == 3) {
        cdc = 0;
        if (isPlaying) { 
          MP3.write(pause, 4); 
          isPlaying = false;
          tr1 = 170;
          report();
        } else {
          isPlaying = true;
          MP3.write(play, 4); 
          track();
          report();
        }
     }

      if (cdc == 6) {
        cdc = 10;
        info = 7;
        tr1 = 222;
        report();
        get_cf();
        pp(400);
        tr1 = cd/10*16 + cd%10; 
        report();
    }

      if (cdc == 9) {
        cdc = 0;
        info = 4;
        tr1 = 221;
        report();
        get_cf();
        pp(400);
        uint8_t tr2 = EEPROM.read(cd);
        if (tr2 > 99) tr2 = 99;
        tr1 = tr2/10*16 + tr2%10; 
        report();
    }
  }
}

void wire() {
  Wire.begin(0x40);
  Wire.onReceive(receiveEvent);
}

void getMP3Status() {
  uint8_t playStatus[] =  {0xAA, 0x01, 0x00, 0xAB};
  MP3.write(playStatus, 4);
  MP3.flush();
  uint8_t pStatus[5] = {0};  
  uint8_t indx = 0; 
  while (!MP3.available()) pp(20);
  while (MP3.available()) {
    pStatus[indx] = MP3.read();
    indx++;
    pp(10);
    } 
isPlay = pStatus[3]; 
}

void get_cf() {
  uint8_t currentSong[] = {0xAA, 0x0D, 0x00, 0xB7};
  uint8_t noTracks[6] = {0};  
  uint8_t indx = 0; 
MP3.write(currentSong, 4);
MP3.flush();
while (!MP3.available()) pp(20);
while (MP3.available()) {
    noTracks[indx] = MP3.read();
    indx++;
    pp(10);
  }
  cf = 256*noTracks[3] + noTracks[4];
}

void playSong() {
   selectSong[3] = cf>>8;
   selectSong[4] = cf;
   selectSong[5] = selectSong[3] + selectSong[4] + 0xCB;
   MP3.write(selectSong, 6);
   MP3.write(play, 4);
}

void track() {
  ff = 0;
  if (cf < 1) cf = fi;
  i = EEPROM.read(0);
  for (cd = 1; cd <= i; cd++) {
    ff = EEPROM.read(cd) + ff;
    if (ff >= cf ) break;
    }
  tr = cf - ff + EEPROM.read(cd);
  cd1 = cd;
  uint8_t tr2 = tr; 
  if (cd > 15) {
  while (cd1 > 10) cd1 -= 10;
  }
  while (tr2 >= 100) {tr2 -= 100;}
  tr1 = tr2/10*16 + tr2%10; 
}

void stat() {
  stp = 0;
  if (isScan) stp = 1;
  if (isRandom) stp = 2;
  if (isScan && isRandom) stp = 3;
  uint8_t st[27]; 
  uint8_t check = stp + 3;
  pp(40);
for (uint8_t a = 0; a < 8; a++) {
  st[7-a] = bitRead(0x03, a);
  st[16-a] = bitRead(stp, a);
  st[25-a] = bitRead(check, a);
  }
st[8] = 1;
st[17] = 1;
st[26] = 1;
ticks(st, 27);
}

void report() {
  uint8_t st[36]; 
  uint16_t check = cd1 + tr1 + 1;
  if ((check & 0x1F0) && ((check - (cd1 & 0x0f)) <= (check & 0x1F0))) {
    check -= 0x10;
  }
  pp(30);
for (uint8_t a = 0; a < 8; a++) {
  st[7-a] = bitRead(0x01, a);
  st[16-a] = bitRead(cd1, a);
  st[25-a] = bitRead(tr1, a);
  st[34-a] = bitRead(check, a);
  }
st[8] = 1;
st[17] = 1;
st[26] = 1;
st[35] = 1;
ticks(st, 36);
}

void pp(uint16_t pau) {
  uint32_t timer3 = millis();
  while (millis() - timer3 < pau);
}

void ticks(uint8_t *arrSt, uint8_t tick) {
  Wire.end();
  DDRC = 0b110000;
  PORTC &= ~(1 << 4); delayMicroseconds(100);
  PORTC &= ~(1 << 5); delayMicroseconds(600);
  uint8_t b = 0;
  uint32_t timer2 = micros();
  uint8_t t = 0;
while (b < tick) {
if (micros() - timer2 >= 300) {
   timer2 = micros();
if (t == 0 && arrSt[b] == 1) PORTC |= (1 << 4);
if (t == 0 && arrSt[b] == 0) PORTC &= ~(1 << 4);
if (t == 1) PORTC |= (1 << 5);
if (t == 3) PORTC &= ~(1 << 5);
   t++;
   if (t == 4) {t = 0; b++;}
  }
}
  PORTC &= ~(1 << 4); delayMicroseconds(600);
  PORTC |= (1 << 5); delayMicroseconds(100);
  PORTC |= (1 << 4);
  pp(20); 
  wire();
}

void receiveEvent(int howMany) {
  while (0 < Wire.available()) {  
    uint8_t x0 = Wire.read();
    if (0 < Wire.available()) {
      uint8_t x1 = Wire.read();
      uint16_t x = ((x0 << 8) + x1);
      switch (x) {
        case HU_START:
        cdc = 15;        
          break;
        case HU_STOP:
        cdc = 16;
          break;
        case HU_LEFT:
        cdc = 13;
          break;
        case HU_RIGHT:
          cdc = 14;
          break;
        case HU_DOWN:
        cdc = 12;
          break;
        case HU_UP:
        cdc = 11;
          break;
        case HU_1:
        cdc = 1;
          break;
        case HU_2:
        cdc = 2;
          break;
        case HU_3:
        cdc = 3;
          break;
        case HU_4:
        cdc = 13;
          break;
        case HU_5:
        cdc = 14;
          break;
        case HU_6:
        cdc = 6;
          break;
        case HU_SCAN:
        cdc = 7;        
          break;
        case HU_RND:
        cdc = 8;        
          break;
      }
    }
  }
}
