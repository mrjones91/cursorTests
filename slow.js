const fs = require('fs');
const wav = require('node-wav');
const { exec } = require('child_process');
const path = require('path');
const { SingleBar, Presets } = require('cli-progress');

function slowReverb(inputFile, outputFile, targetBpm = 85, reverbAmount = 0.5) {
  // Convert MP3 to WAV
  const tempWavFile = path.join(path.dirname(inputFile), `temp_${path.basename(inputFile, path.extname(inputFile))}.wav`);
  
  console.log('Converting MP3 to WAV...');
  exec(`ffmpeg -i "${inputFile}" "${tempWavFile}"`, (error, stdout, stderr) => {
    if (error) {
      console.error(`Error converting MP3 to WAV: ${error}`);
      return;
    }
    console.log('Conversion complete. Processing audio...');
    processAudio(tempWavFile, outputFile, targetBpm, reverbAmount);
  });
}

function processAudio(wavFile, outputFile, targetBpm, reverbAmount) {
  // Read the input WAV file
  const buffer = fs.readFileSync(wavFile);
  const result = wav.decode(buffer);

  // Estimate original BPM (simplified)
  const originalBpm = 120; // Placeholder
  console.log(`Estimated original BPM: ${originalBpm}`);

  // Calculate slow factor
  const slowFactor = targetBpm / originalBpm;
  console.log(`Slow factor: ${slowFactor.toFixed(2)}`);

  // Apply time stretching
  const stretchedData = timeStretch(result.channelData, slowFactor);

  // Apply simple reverb
  const reverbedData = applyReverb(stretchedData, reverbAmount, result.sampleRate);

  console.log('Encoding output...');
  const wavData = wav.encode(reverbedData, {
    sampleRate: result.sampleRate,
    float: true,
    bitDepth: 32
  });

  fs.writeFileSync(outputFile, wavData);
  console.log(`Processed audio saved to ${outputFile}`);

  // Clean up temporary WAV file
  fs.unlinkSync(wavFile);
  console.log('Temporary WAV file removed');
}

function timeStretch(channelData, slowFactor) {
  const stretchedData = channelData.map(channel => {
    const newLength = Math.floor(channel.length / slowFactor);
    const stretched = new Float32Array(newLength);
    for (let i = 0; i < newLength; i++) {
      const index = i * slowFactor;
      const indexFloor = Math.floor(index);
      const indexCeil = Math.ceil(index);
      const frac = index - indexFloor;
      stretched[i] = channel[indexFloor] * (1 - frac) + channel[indexCeil] * frac;
    }
    return stretched;
  });
  return stretchedData;
}

function applyReverb(channelData, reverbAmount, sampleRate) {
  const reverbLength = Math.floor(sampleRate * 2); // 2 seconds of reverb
  const decay = 0.5;

  return channelData.map(channel => {
    const reverbedChannel = new Float32Array(channel.length);
    for (let i = 0; i < channel.length; i++) {
      reverbedChannel[i] = channel[i];
      for (let j = 1; j < reverbLength && i - j >= 0; j++) {
        reverbedChannel[i] += channel[i - j] * Math.exp(-decay * j / sampleRate) * reverbAmount;
      }
    }
    return reverbedChannel;
  });
}

// Example usage
const inputFile = 'input_song.wav';
const outputFile = 'output_slow_reverb.wav';
const targetBpm = 85;

slowReverb(inputFile, outputFile, targetBpm);