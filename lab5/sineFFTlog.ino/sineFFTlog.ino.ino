//Yueyang Cheng(1533989)
//lab#5 part 4
// a log approximation of FFT data
#include <analyze_fft1024.h>
#include <analyze_fft256.h>
#include <analyze_notefreq.h>
#include <analyze_peak.h>
#include <analyze_print.h>
#include <analyze_rms.h>
#include <analyze_tonedetect.h>
#include <Audio.h>
#include <AudioControl.h>
#include <control_ak4558.h>
#include <control_cs42448.h>
#include <control_cs4272.h>
#include <control_sgtl5000.h>
#include <control_wm8731.h>
#include <effect_bitcrusher.h>
#include <effect_chorus.h>
#include <effect_delay.h>
#include <effect_delay_ext.h>
#include <effect_envelope.h>
#include <effect_fade.h>
#include <effect_flange.h>
#include <effect_midside.h>
#include <effect_multiply.h>
#include <effect_reverb.h>
#include <effect_waveshaper.h>
#include <filter_biquad.h>
#include <filter_fir.h>
#include <filter_variable.h>
#include <input_adc.h>
#include <input_adcs.h>
#include <input_i2s.h>
#include <input_i2s_quad.h>
#include <input_tdm.h>
#include <memcpy_audio.h>
#include <mixer.h>
#include <output_dac.h>
#include <output_dacs.h>
#include <output_i2s.h>
#include <output_i2s_quad.h>
#include <output_pt8211.h>
#include <output_pwm.h>
#include <output_spdif.h>
#include <output_tdm.h>
#include <play_memory.h>
#include <play_queue.h>
#include <play_sd_raw.h>
#include <play_sd_wav.h>
#include <play_serialflash_raw.h>
#include <record_queue.h>
#include <spi_interrupt.h>
#include <synth_dc.h>
#include <synth_karplusstrong.h>
#include <synth_pinknoise.h>
#include <synth_pwm.h>
#include <synth_simple_drum.h>
#include <synth_sine.h>
#include <synth_tonesweep.h>
#include <synth_waveform.h>
#include <synth_whitenoise.h>

// FFT Test
//
// Compute a 1024 point Fast Fourier Transform (spectrum analysis)
// on audio connected to the Left Line-In pin.  By changing code,
// a synthetic sine wave can be input instead.
//
// The first 40 (of 512) frequency analysis bins are printed to
// the Arduino Serial Monitor.  Viewing the raw data can help you
// understand how the FFT works and what results to expect when
// using the data to control LEDs, motors, or other fun things!
//
// This example code is in the public domain.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs

// GUItool: begin automatically generated code
AudioSynthWaveformSine   sine1;          //xy=249,132
AudioInputAnalog         adc1;           //xy=251,179
AudioOutputAnalog        dac1;           //xy=551,183
AudioAnalyzeFFT1024      fft1024_1;      //xy=560,110
// Connect either the live input or synthesized sine wave
//AudioConnection          patchCord1(adc1, fft1024_1);
//AudioConnection          patchCord2(adc1, dac1);
AudioConnection          patchCord1(sine1, fft1024_1);
AudioConnection          patchCord2(sine1, dac1);
// GUItool: end automatically generated code



//AudioControlSGTL5000 audioShield;

void setup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(12);

  // Enable the audio shield and set the output volume.
//  audioShield.enable();
//  audioShield.inputSelect(myInput);
//  audioShield.volume(0.5);

  // Configure the window algorithm to use
  fft1024_1.windowFunction(AudioWindowHanning1024);
  //fft1024_1.windowFunction(NULL);

  // Create a synthetic sine wave, for testing
  // To use this, edit the connections above
  sine1.amplitude(0.8);
  sine1.frequency(1034.007);
}

void loop() {
  float n, level[16];
  int i;

  if (fft1024_1.available()) {
    // each time new FFT data is available
    // apply log approximation
    level[0] = fft1024_1.read(0);
    level[1] = fft1024_1.read(1);
    level[2] = fft1024_1.read(2, 3);
    level[3] = fft1024_1.read(4, 6);
    level[4] = fft1024_1.read(7, 10);
    level[5] = fft1024_1.read(11, 15);
    level[6] = fft1024_1.read(16, 22);
    level[7] = fft1024_1.read(23, 32);
    level[8] = fft1024_1.read(33, 46);
    level[9] = fft1024_1.read(47, 66);
    level[10] = fft1024_1.read(67, 93);
    level[11] = fft1024_1.read(94, 131);
    level[12] = fft1024_1.read(132, 184);
    level[13] = fft1024_1.read(185, 257);
    level[14] = fft1024_1.read(258, 359);
    level[15] = fft1024_1.read(360, 511);
    // print it all to the Arduino Serial Monitor
    Serial.print("FFT: ");
    for (i=0; i<16; i++) {
      n = level[i];
      if (n >= 0.01) {
        Serial.print(n);
        Serial.print(" ");
      } else {
        Serial.print("  -  "); // don't print "0.00"
      }
    }
    Serial.println();
  }
}


