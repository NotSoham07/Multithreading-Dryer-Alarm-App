#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define ADC_PATH "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define RUNNING_THRESHOLD 500  // Set based on your observations

int read_adc() {
    int fd;
    char buf[64];
    int value;

    fd = open(ADC_PATH, O_RDONLY);  // Open the ADC device file
    if (fd < 0) {
        perror("Error opening ADC device");
        return -1;
    }

    if (read(fd, buf, sizeof(buf)) == -1) {
        perror("Error reading ADC value");
        close(fd);
        return -1;
    }

    value = atoi(buf);  // Convert the read string to integer
    close(fd);

    return value;
}

int main() {
    int isRunning = 0;

    while (1) {
        int adc_value = read_adc();
        printf("ADC Value: %d\n", adc_value);  // Print the ADC value
        
        if (adc_value < RUNNING_THRESHOLD) {
            if (!isRunning) {
                printf("Dryer is running.\n");
                isRunning = 1;
            }
        } else {
            if (isRunning) {
                printf("Dryer has stopped.\n");
                isRunning = 0;
            }
        }
        usleep(100000);  // Delay for 100 milliseconds
    }

    return 0;
}
