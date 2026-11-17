#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#define FEATURES 3
#define LATENT 2
#define Q7_SCALE 127
#define THRESHOLD 900

static const int8_t W_ENC[LATENT][FEATURES] = {
    {127, 0, 0},
    {0, 127, 0}
};

static const int32_t B_ENC[LATENT] = {0, 0};

static const int8_t W_DEC[FEATURES][LATENT] = {
    {127, 0},
    {0, 127},
    {64, 64}
};


static const int32_t B_DEC[FEATURES] = {0, 0, 0};

static int8_t clamp_i8(int value) {
    if (value > 127) return 127;
    if (value < -127) return -127;
    return (int8_t)value;
}

static int8_t normalize_sensor(float value) {
    if (value < 0.0f) value = 0.0f;
    if (value > 100.0f) value = 100.0f;
    return clamp_i8((int)lroundf(value * 1.27f));
}

static void dense_q7(const int8_t *input, int input_size,
                     const int8_t *weights, const int32_t *bias,
                     int output_size, int8_t *output) {
    for (int o = 0; o < output_size; ++o) {
        int32_t acc = bias[o];
        for (int i = 0; i < input_size; ++i)
            acc += (int32_t)input[i] * (int32_t)weights[o * input_size + i];
        acc = acc / Q7_SCALE;
        output[o] = clamp_i8((int)acc);
    }
}

void play_alert(void) {
    for (int i = 0; i < 10; i++) {
        printf("\a");
        fflush(stdout);
        usleep(500000);
    }
}

static int detect_anomaly(float temperature, float vibration, float current) {
    int8_t x[FEATURES] = {
        normalize_sensor(temperature),
        normalize_sensor(vibration),
        normalize_sensor(current)
    };
    int8_t z[LATENT];
    int8_t reconstructed[FEATURES];

    dense_q7(x, FEATURES, &W_ENC[0][0], B_ENC, LATENT, z);
    dense_q7(z, LATENT, &W_DEC[0][0], B_DEC, FEATURES, reconstructed);

    int32_t sum_sq = 0;
    
    for (int i = 0; i < FEATURES; ++i) {
        int32_t diff = (int32_t)x[i] - (int32_t)reconstructed[i];
        sum_sq += diff * diff;
    }
    int32_t mse = sum_sq / FEATURES;
    
    if (mse > THRESHOLD) {
        printf("RESULT: ANOMALY DETECTED\n");
        play_alert();
        return 1;
    }
    printf("RESULT: NORMAL\n");
    return 0;
}

int main(void) {
    int choice;
    float t, v, c;
        printf("Temperature (0-100): ");
        if (scanf("%f", &t) != 1) return 1;
        printf("Vibration   (0-100): ");
        if (scanf("%f", &v) != 1) return 1;
        printf("Current     (0-100): ");
        if (scanf("%f", &c) != 1) return 1;
        detect_anomaly(t, v, c);
    puts("C-TinyGuard stopped.");
    return 0;
}
