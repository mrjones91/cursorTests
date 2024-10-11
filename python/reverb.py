import librosa
import soundfile as sf
import numpy as np

def slow_reverb(input_file, output_file, slow_factor=0.8, reverb_amount=0.5):
    # Load the audio file
    y, sr = librosa.load(input_file)

    # Slow down the audio
    y_slow = librosa.effects.time_stretch(y, rate=slow_factor)

    # Apply reverb (simple convolution with exponential decay)
    reverb_len = int(sr * 2)  # 2 seconds of reverb
    reverb = np.exp(-np.linspace(0, 5, reverb_len))
    y_reverb = np.convolve(y_slow, reverb, mode='full')[:len(y_slow)]

    # Mix the original slowed audio with the reverb
    y_output = y_slow + reverb_amount * y_reverb

    # Normalize the output
    y_output = y_output / np.max(np.abs(y_output))

    # Write the output file
    sf.write(output_file, y_output, sr)

# Example usage
input_file = "input_song.mp3"
output_file = "output_slow_reverb.wav"
slow_reverb(input_file, output_file)
