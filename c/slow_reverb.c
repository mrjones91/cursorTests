#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sndfile.h>
#include <fftw3.h>

#define CHUNK_SIZE 44100
#define OVERLAP 1000
#define REVERB_LENGTH 88200  // 2 seconds at 44.1kHz

void apply_reverb(float *input, float *output, int input_length, float *reverb, int reverb_length) {
    fftwf_complex *fft_input, *fft_reverb, *fft_result;
    fftwf_plan plan_forward_input, plan_forward_reverb, plan_backward;
    int fft_size = input_length + reverb_length - 1;

    fft_input = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fft_size);
    fft_reverb = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fft_size);
    fft_result = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fft_size);

    plan_forward_input = fftwf_plan_dft_r2c_1d(fft_size, input, fft_input, FFTW_ESTIMATE);
    plan_forward_reverb = fftwf_plan_dft_r2c_1d(fft_size, reverb, fft_reverb, FFTW_ESTIMATE);
    plan_backward = fftwf_plan_dft_c2r_1d(fft_size, fft_result, output, FFTW_ESTIMATE);

    fftwf_execute(plan_forward_input);
    fftwf_execute(plan_forward_reverb);

    for (int i = 0; i < fft_size / 2 + 1; i++) {
        fft_result[i][0] = fft_input[i][0] * fft_reverb[i][0] - fft_input[i][1] * fft_reverb[i][1];
        fft_result[i][1] = fft_input[i][0] * fft_reverb[i][1] + fft_input[i][1] * fft_reverb[i][0];
    }

    fftwf_execute(plan_backward);

    for (int i = 0; i < input_length; i++) {
        output[i] /= fft_size;
    }

    fftwf_destroy_plan(plan_forward_input);
    fftwf_destroy_plan(plan_forward_reverb);
    fftwf_destroy_plan(plan_backward);
    fftwf_free(fft_input);
    fftwf_free(fft_reverb);
    fftwf_free(fft_result);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input_file> <output_file> <target_bpm>\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];
    float target_bpm = atof(argv[3]);

    SNDFILE *infile, *outfile;
    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(sfinfo));

    infile = sf_open(input_file, SFM_READ, &sfinfo);
    if (!infile) {
        printf("Error opening input file.\n");
        return 1;
    }

    float *buffer = malloc(sfinfo.frames * sizeof(float));
    sf_read_float(infile, buffer, sfinfo.frames);
    sf_close(infile);

    // Estimate BPM (simplified, not accurate)
    float original_bpm = 120.0;  // Placeholder
    float slow_factor = target_bpm / original_bpm;

    // Slow down audio (simplified time-stretching)
    int new_length = (int)(sfinfo.frames / slow_factor);
    float *slowed = malloc(new_length * sizeof(float));
    for (int i = 0; i < new_length; i++) {
        int orig_index = (int)(i * slow_factor);
        slowed[i] = buffer[orig_index];
    }

    // Create reverb impulse
    float *reverb = malloc(REVERB_LENGTH * sizeof(float));
    for (int i = 0; i < REVERB_LENGTH; i++) {
        reverb[i] = exp(-5.0 * i / REVERB_LENGTH);
    }

    // Apply reverb
    float *output = malloc(new_length * sizeof(float));
    memset(output, 0, new_length * sizeof(float));

    for (int i = 0; i < new_length; i += CHUNK_SIZE - OVERLAP) {
        int chunk_length = (i + CHUNK_SIZE > new_length) ? (new_length - i) : CHUNK_SIZE;
        float *chunk_reverb = malloc(chunk_length * sizeof(float));
        
        apply_reverb(slowed + i, chunk_reverb, chunk_length, reverb, REVERB_LENGTH);

        // Crossfade
        for (int j = 0; j < chunk_length; j++) {
            if (i > 0 && j < OVERLAP) {
                float fade_out = (float)(OVERLAP - j) / OVERLAP;
                float fade_in = (float)j / OVERLAP;
                output[i + j] = output[i + j] * fade_out + chunk_reverb[j] * fade_in;
            } else {
                output[i + j] = chunk_reverb[j];
            }
        }

        free(chunk_reverb);
    }

    // Mix original slowed audio with reverb
    for (int i = 0; i < new_length; i++) {
        output[i] = slowed[i] + 0.5 * output[i];
    }

    // Normalize output
    float max_sample = 0.0;
    for (int i = 0; i < new_length; i++) {
        if (fabs(output[i]) > max_sample) {
            max_sample = fabs(output[i]);
        }
    }
    for (int i = 0; i < new_length; i++) {
        output[i] /= max_sample;
    }

    // Write output
    outfile = sf_open(output_file, SFM_WRITE, &sfinfo);
    if (!outfile) {
        printf("Error opening output file.\n");
        return 1;
    }
    sf_write_float(outfile, output, new_length);
    sf_close(outfile);

    free(buffer);
    free(slowed);
    free(reverb);
    free(output);

    printf("Processed audio saved to %s\n", output_file);

    return 0;
}