#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define VIBRATION_SENSOR_ADC_PATH "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define LIGHT_SENSOR_ADC_PATH "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
#define BUZZER_GPIO_PATH "/sys/class/gpio/gpio68/value"
#define RED_LED_GPIO_PATH "/sys/class/gpio/gpio66/value"
#define GREEN_LED_GPIO_PATH "/sys/class/gpio/gpio67/value"

#define VIBRATION_THRESHOLD 3950  // Adjust based on actual vibration levels when dryer is running
#define LIGHT_THRESHOLD 10        // Buzzer activates when light value is greater than 10

int read_adc(const char* adc_path) {
    int fd;
    char buf[64];
    int value;

    fd = open(adc_path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening ADC device");
        return -1;
    }

    ssize_t count = read(fd, buf, sizeof(buf)-1);
    if (count == -1) {
        perror("Error reading ADC value");
        close(fd);
        return -1;
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

void setup_gpio(const char* gpio_number, const char* gpio_path) {
    int fd;
    char path[50];

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%s/direction", gpio_number);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open GPIO direction for writing");
        return;
    }
    if (write(fd, "out", 3) != 3) {
        perror("Error writing to GPIO direction");
    }
    close(fd);
}

int main() {
    printf("Starting the dryer alarm system...\n");

    while (1) {
        int vibration_value = read_adc(VIBRATION_SENSOR_ADC_PATH);
        int light_value = read_adc(LIGHT_SENSOR_ADC_PATH);

        printf("Vibration Sensor Value: %d, Light Sensor Value: %d\n", vibration_value, light_value);

        if (vibration_value > VIBRATION_THRESHOLD) {
            write_gpio(RED_LED_GPIO_PATH, "1");    // Turn on the red LED
            write_gpio(GREEN_LED_GPIO_PATH, "0"); // Turn off the green LED
            printf("Dryer is running. Red LED on.\n");
        } else {
            write_gpio(RED_LED_GPIO_PATH, "0");   // Turn off the red LED
            write_gpio(GREEN_LED_GPIO_PATH, "1"); // Turn on the green LED
            printf("Dryer is done. Green LED on.\n");
        }

        // Buzzer logic based on light and vibration
        if (vibration_value > VIBRATION_THRESHOLD && light_value > LIGHT_THRESHOLD) {
            write_gpio(BUZZER_GPIO_PATH, "1");  // Activate the buzzer
            printf("Conditions met: Buzzer activated!\n");
        } else {
            write_gpio(BUZZER_GPIO_PATH, "0");  // Deactivate the buzzer
            printf("Conditions not met: Buzzer deactivated.\n");
        }

        usleep(500000);  // Delay for 500 milliseconds
    }

    return 0;
}
