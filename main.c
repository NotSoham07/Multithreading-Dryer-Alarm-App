#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define ADC_VIBRATION_PATH "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define ADC_LIGHT_PATH "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
#define BUZZER_GPIO_PATH "/sys/class/gpio/gpio68/value"  // Example GPIO path for the buzzer

#define VIBRATION_THRESHOLD 500
#define LIGHT_THRESHOLD 150

int read_adc(const char* adc_path) {
    int fd;
    char buf[64];
    int value;

    fd = open(adc_path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening ADC device");
        return -1;
    }

    if (read(fd, buf, sizeof(buf)) == -1) {
        perror("Error reading ADC value");
        close(fd);
        return -1;
    }

    value = atoi(buf);
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

int main() {
    while (1) {
        int vibration_value = read_adc(ADC_VIBRATION_PATH);
        int light_value = read_adc(ADC_LIGHT_PATH);

        if (vibration_value < VIBRATION_THRESHOLD && light_value < LIGHT_THRESHOLD) {
            write_gpio(BUZZER_GPIO_PATH, "1");  // Turn on the buzzer
            printf("Dryer is running, and lights are on. Buzzer activated!\n");
        } else {
            write_gpio(BUZZER_GPIO_PATH, "0");  // Turn off the buzzer
        }

        usleep(100000);  // Delay for 100 milliseconds
    }

    return 0;
}
