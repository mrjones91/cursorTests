import librosa
import soundfile as sf
import numpy as np
from tqdm import tqdm

def slow_reverb(input_file, output_file, target_bpm=100, reverb_amount=0.5):
    # Load the audio file
    y, sr = librosa.load(input_file)

    # Estimate the original BPM
    print("Estimating BPM...")
    tempo, _ = librosa.beat.beat_track(y=y, sr=sr)
    original_bpm = tempo[0] if isinstance(tempo, np.ndarray) else tempo
    print(f"Estimated original BPM: {original_bpm:.2f}")

    # Calculate the slow factor
    slow_factor = target_bpm / original_bpm
    print(f"Slow factor: {slow_factor:.2f}")

    # Slow down the audio
    print("Slowing down audio...")
    y_slow = librosa.effects.time_stretch(y, rate=slow_factor)

    # Apply reverb (simple convolution with exponential decay)
    print("Applying reverb...")
    reverb_len = int(sr * 2)  # 2 seconds of reverb
    reverb = np.exp(-np.linspace(0, 5, reverb_len))
    
    # Use tqdm for the convolution process
    y_reverb = np.zeros_like(y_slow)
    chunk_size = 44100  # 1 second of audio at 44.1kHz
    overlap = 1000  # Overlap between chunks for crossfading

    for i in tqdm(range(0, len(y_slow), chunk_size - overlap), desc="Applying reverb"):
        chunk = y_slow[i:i+chunk_size]
        if len(chunk) < chunk_size:
            chunk = np.pad(chunk, (0, chunk_size - len(chunk)), mode='constant')
        
        convolved = np.convolve(chunk, reverb, mode='same')
        
        if i > 0:
            # Apply crossfade
            fade_in = np.linspace(0, 1, overlap)
            fade_out = np.linspace(1, 0, overlap)
            y_reverb[i:i+overlap] *= fade_out
            y_reverb[i:i+overlap] += convolved[:overlap] * fade_in
            y_reverb[i+overlap:i+chunk_size] = convolved[overlap:]
        else:
            y_reverb[i:i+chunk_size] = convolved

    # Trim y_reverb to match y_slow length
    y_reverb = y_reverb[:len(y_slow)]

    # Mix the original slowed audio with the reverb
    print("Mixing slowed audio with reverb...")
    y_output = y_slow + reverb_amount * y_reverb

    # Normalize the output
    print("Normalizing output...")
    y_output = y_output / np.max(np.abs(y_output))

    # Write the output file
    print(f"Saving to {output_file}...")
    sf.write(output_file, y_output, sr)

    print(f"Processed audio saved to {output_file}")

# Example usage
input_file = "input_song.mp3"
output_file = "output.wav"
target_bpm = 100  # Adjust this to your desired BPM
slow_reverb(input_file, output_file, target_bpm)