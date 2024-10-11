import librosa
import soundfile as sf
import numpy as np

def slow_reverb(input_file, output_file, target_bpm=85, reverb_amount=0.5):
    # Load the audio file
    y, sr = librosa.load(input_file)

    # Estimate the original BPM
    tempo, _ = librosa.beat.beat_track(y=y, sr=sr)
    original_bpm = tempo[0] if isinstance(tempo, np.ndarray) else tempo
    print(f"Estimated original BPM: {original_bpm:.2f}")

    # Calculate the slow factor
    slow_factor = target_bpm / original_bpm
    print(f"Slow factor: {slow_factor:.2f}")

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

    print(f"Processed audio saved to {output_file}")

# Example usage
input_file = "input_song.mp3"
output_file = "output_slow_reverb.wav"
target_bpm = 85  # Adjust this to your desired BPM
slow_reverb(input_file, output_file, target_bpm)