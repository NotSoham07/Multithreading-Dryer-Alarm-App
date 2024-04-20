#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define LIGHT_SENSOR_ADC_PATH "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"

int read_light_sensor() {
    int fd;
    char buf[64];
    int value;

    fd = open(LIGHT_SENSOR_ADC_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Error opening light sensor ADC device");
        return -1;
    }

    if (read(fd, buf, sizeof(buf)) == -1) {
        perror("Error reading light sensor ADC value");
        close(fd);
        return -1;
    }

    value = atoi(buf);
    close(fd);
    return value;
}

int main() {
    while (1) {
        int light_value = read_light_sensor();
        printf("Light Sensor ADC Value: %d\n", light_value);
        usleep(100000);  // Delay for 100 milliseconds
    }

    return 0;
}
