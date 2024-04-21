#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define VIBRATION_SENSOR_ADC_PATH "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define LIGHT_SENSOR_ADC_PATH "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
#define BUZZER_GPIO_PATH "/sys/class/gpio/gpio68/value"
#define RED_LED_GPIO_PATH "/sys/class/gpio/gpio66/value"
#define GREEN_LED_GPIO_PATH "/sys/class/gpio/gpio67/value"

#define VIBRATION_THRESHOLD 3250
#define LIGHT_THRESHOLD 10

volatile int vibration_value = 0;
volatile int light_value = 0;
pthread_mutex_t lock;

int read_adc(const char* adc_path) {
    int fd;
    char buf[64];
    int value = -1;

    fd = open(adc_path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening ADC device");
        return -1;
    }

    ssize_t count = read(fd, buf, sizeof(buf) - 1);
    if (count == -1) {
        perror("Error reading ADC value");
        close(fd);
    } else {
        buf[count] = '\0';
        value = atoi(buf);
    }

    close(fd);
    return value;
}

void write_gpio(const char* gpio_path, const char* value) {
    int fd = open(gpio_path, O_WRONLY);
    if (fd < 0) {
        perror("Error opening GPIO device");
        return;
    }
    if (write(fd, value, strlen(value)) != strlen(value)) {
        perror("Error writing GPIO value");
    }
    close(fd);
}

void* sensor_reading_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&lock);
        vibration_value = read_adc(VIBRATION_SENSOR_ADC_PATH);
        light_value = read_adc(LIGHT_SENSOR_ADC_PATH);
        pthread_mutex_unlock(&lock);
        usleep(100000); // 100 ms
    }
    return NULL;
}

void* led_control_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&lock);
        if (vibration_value < VIBRATION_THRESHOLD) {
            write_gpio(RED_LED_GPIO_PATH, "1");
            write_gpio(GREEN_LED_GPIO_PATH, "0");
        } else {
            write_gpio(RED_LED_GPIO_PATH, "0");
            write_gpio(GREEN_LED_GPIO_PATH, "1");
        }
        pthread_mutex_unlock(&lock);
        usleep(100000); // 100 ms
    }
    return NULL;
}

void* buzzer_control_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&lock);
        if (vibration_value > VIBRATION_THRESHOLD && light_value > LIGHT_THRESHOLD) {
            write_gpio(BUZZER_GPIO_PATH, "1");
        } else {
            write_gpio(BUZZER_GPIO_PATH, "0");
        }
        pthread_mutex_unlock(&lock);
        usleep(100000); // 100 ms
    }
    return NULL;
}

int main() {
    printf("Starting the dryer alarm system...\n");
    pthread_t threads[3];
    pthread_mutex_init(&lock, NULL);

    pthread_create(&threads[0], NULL, sensor_reading_thread, NULL);
    pthread_create(&threads[1], NULL, led_control_thread, NULL);
    pthread_create(&threads[2], NULL, buzzer_control_thread, NULL);

    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);
    return 0;
}
