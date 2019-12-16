#include <Adafruit_VS1053.h>
#include <SPI.h>
#include <SD.h>

#define SHIELD_RESET -1
#define SHIELD_CS 7
#define SHIELD_DCS 6
#define CARDCS 4
#define DREQ 3

#define ARR_SIZE(arr) ( sizeof((arr)) / sizeof((arr[0])) )

// random
const int randInput = 0;

// time
unsigned long time;
unsigned long lastRange;
unsigned long lastMusic;
unsigned long lastKill;

const long rangeInterval = 1000;
const long musicInterval = 1000;
const long killInterval = 1000;

// death
int killLoops = 0;
int maxKillLoops = 3;

// button
int buttonInput = 8;
int buttonState = 0;
bool dead = false;
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// range
const int rangeInput = 1;
long inches;

// music
bool musicPlaying;
long randTrack;
char boot[] = "/BOOT.mp3";
char killed[] = "/DEAD.mp3";
char track1[] = "/TRACK1.mp3";
char track2[] = "/TRACK2.mp3";
char track3[] = "/TRACK3.mp3";
char track4[] = "/TRACK4.mp3";
char * tracks[] =
{
  track1,
  track2,
  track3,
  track4
};
Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

void(* resetFunc) (void) = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting");

  // Setup random
  randomSeed(analogRead(randInput));

  // setup time
  time = millis();
  lastMusic = time;
  lastRange = time;
  lastKill = time;

  // setup button
  pinMode(buttonInput, INPUT);
  dead = false;

  // setup music
  if (! musicPlayer.begin()) {
    Serial.println(F("Error starting VS1053"));
    while (1);
  }

  if (!SD.begin(CARDCS)) {
    Serial.println(F("Error loading SD"));
    while (1);
  }

  musicPlayer.setVolume(20, 20);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);
  musicPlayer.startPlayingFile(boot);
  musicPlaying = false;
}

// Play sync
// musicPlayer.playFullFile("/full/path/to/file.mp3");
// Play async
// musicPlayer.startPlayingFile("/full/path/to/file.mp3");

void loop() {
  time = millis();

  checkButton();
  checkRange();
  checkMusic();
  checkDead();
}

void checkDead() {
  if (dead) {
    if (musicPlayer.stopped()) {
      musicPlayer.playFullFile(killed);
    }

    if (time - lastKill >= killInterval) {
      lastKill = time;
      Serial.print("kill loop=");
      Serial.println(killLoops);
      killLoops++;
    }

    if (killLoops >= maxKillLoops) {
      Serial.println("restarting");
      delay(3500);
      resetFunc();
    }
  }
}

void checkButton() {
  if (time < 3000) {
    return;
  }
  int reading = digitalRead(buttonInput);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == HIGH) {
        if (!dead) {
          dead = true;
          Serial.println("killed");
        }
      }
    }
  }
  lastButtonState = reading;
}

void checkRange() {
  if (dead) {
    return;
  }
  if (time - lastRange >= rangeInterval) {
    lastRange = time;

    long anVolt = analogRead(rangeInput);
    inches = anVolt / 2;
    Serial.print("range inches=");
    Serial.println(inches);
    if (inches > 12 && inches < 24) {
      maybePlayMusic();
    }
  }
}

void checkMusic() {
  if (dead) {
    return;
  }
  if (time - lastMusic >= musicInterval) {
    lastMusic = time;

    if (musicPlayer.stopped()) {
      if (musicPlaying) {
        Serial.println("music stopped");
        musicPlaying = false;
      }
    } else {
      if (!musicPlaying) {
        Serial.println("music started");
        musicPlaying = true;
      }
    }
  }
}

void maybePlayMusic() {
  if (dead) {
    return;
  }
  if (musicPlayer.stopped()) {
    randTrack = random(0, ARR_SIZE(tracks));
    Serial.print("playing track ");
    Serial.println(tracks[randTrack]);
    musicPlayer.startPlayingFile(tracks[randTrack]);
  }
}
