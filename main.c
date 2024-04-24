#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h> 
#include <string.h>
#include <pthread.h>

#define VIBRATION_SENSOR_ADC_PATH "/sys/bus/iio/devices/iio:device0/in_voltage0_raw" // Path to the vibration sensor ADC value
#define LIGHT_SENSOR_ADC_PATH "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"   // Path to the light sensor ADC value
#define BUZZER_GPIO_PATH "/sys/class/gpio/gpio68/value"                            // Path to the buzzer GPIO value
#define RED_LED_GPIO_PATH "/sys/class/gpio/gpio66/value"                          // Path to the red LED GPIO value
#define GREEN_LED_GPIO_PATH "/sys/class/gpio/gpio67/value"                       // Path to the green LED GPIO value

#define VIBRATION_THRESHOLD 3250  // Dryer is running below this threshold, done above
#define LIGHT_THRESHOLD 10        // Buzzer activates when light value is greater than 10

volatile int vibration_value = 0; // Volatile keyword to prevent compiler optimization
volatile int light_value = 0;    // Volatile keyword to prevent compiler optimization
pthread_mutex_t lock;          // Mutex lock

int read_adc(const char* adc_path) { // Function to read the ADC value
    int fd; // File descriptor
    char buf[64]; // Buffer to store the ADC value
    int value = -1; // Default value

    fd = open(adc_path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening ADC device"); 
        return -1;
    }

    ssize_t count = read(fd, buf, sizeof(buf)-1); // Read the ADC value
    if (count == -1) { // Error handling
        perror("Error reading ADC value");
        close(fd);
    } else {
        buf[count] = '\0';
        value = atoi(buf);
    }

    close(fd);
    return value;
}

void write_gpio(const char* gpio_path, const char* value) { // Function to write to the GPIO value
    int fd = open(gpio_path, O_WRONLY); // File descriptor
    if (fd < 0) {
        perror("Error opening GPIO device");
        return;
    }
    if (write(fd, value, strlen(value)) != strlen(value)) {
        perror("Error writing GPIO value");
    }
    close(fd);
}

void setup_gpio(const char* gpio_number) { // Function to setup the GPIO
    char path[50]; // Path to the GPIO
    int fd;

    // Unexport GPIO first to clear any existing configurations
    snprintf(path, sizeof(path), "/sys/class/gpio/unexport"); // Unexport the GPIO
    fd = open(path, O_WRONLY);
    if (fd != -1) {
        write(fd, gpio_number, strlen(gpio_number));
        close(fd);
    }

    // Export GPIO
    snprintf(path, sizeof(path), "/sys/class/gpio/export");
    fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("Failed to open export for writing");
        return;
    }
    if (write(fd, gpio_number, strlen(gpio_number)) != strlen(gpio_number)) { // Write the GPIO number
        perror("Failed to export GPIO");
        close(fd);
        return;
    }
    close(fd);

    // Ensure the GPIO files are ready
    usleep(100000);

    // Set direction to 'out'
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%s/direction", gpio_number);
    fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("Failed to open GPIO direction for writing");
        return;
    }
    if (write(fd, "out", 3) != 3) {
        perror("Failed to set direction");
    }
    close(fd);
}

void* email_notification_thread(void* arg) { // Function to send email notifications
    static int timer_running = 0; // Static variable to keep track of the timer state
    static time_t start_time; // Static variable to store the start time of the timer
    
    while (1) {
        pthread_mutex_lock(&lock);
        if (vibration_value > VIBRATION_THRESHOLD) { // Check if the dryer is done
            if (!timer_running) {
                // Start the timer when the dryer is first detected as done
                start_time = time(NULL);
                timer_running = 1;
                printf("Timer started for email notification.\n");
            } else {
                // Calculate time elapsed since the timer started
                double elapsed = difftime(time(NULL), start_time); // Calculate the time difference
                if (elapsed >= 30) {
                    // Send an email if the dryer is done for more than 30 seconds
                    system("echo 'The dryer has been done for more than 30 seconds.' | mail -s 'Dryer Done Notification' notthesoham0711@gmail.com");
                    printf("Email notification sent after dryer was done for 30 seconds.\n");
                    timer_running = 0;  // Reset the timer to prevent repeated emails
                } else {
                    // Print how many seconds are left until the email is sent
                    printf("Time until notification: %.0f seconds remaining.\n", 30 - elapsed);
                }
            }
        } else {
            // Reset the timer if the dryer is not done
            if (timer_running) {
                printf("Timer reset as dryer is no longer in 'done' state.\n");
            }
            timer_running = 0; // Reset the timer
        }
        pthread_mutex_unlock(&lock); // Unlock the mutex
        sleep(1); // Check every second
    }
    return NULL;
}

void* sensor_reading_thread(void* arg) { // Function to read the sensor values
    while (1) {
        pthread_mutex_lock(&lock); // Lock the mutex
        vibration_value = read_adc(VIBRATION_SENSOR_ADC_PATH); // Read the vibration sensor value
        light_value = read_adc(LIGHT_SENSOR_ADC_PATH); // Read the light sensor value
        pthread_mutex_unlock(&lock);
        printf("Readings - Vibration: %d, Light: %d\n", vibration_value, light_value); // Print the sensor values
        usleep(100000); // 100 ms
    }
    return NULL;
}

void* led_control_thread(void* arg) { // Function to control the LEDs
    while (1) {
        pthread_mutex_lock(&lock);
        if (vibration_value < VIBRATION_THRESHOLD) { // Change the condition to turn on the red LED when the dryer is running
            write_gpio(RED_LED_GPIO_PATH, "1");
            write_gpio(GREEN_LED_GPIO_PATH, "0");
            printf("Dryer is running. Red LED on.\n");
        } else { // Change the condition to turn on the green LED when the dryer is done
            write_gpio(RED_LED_GPIO_PATH, "0"); 
            write_gpio(GREEN_LED_GPIO_PATH, "1");
            printf("Dryer is done. Green LED on.\n");
        }
        pthread_mutex_unlock(&lock);
        usleep(100000); // 100 ms
    }
    return NULL;
}

void* buzzer_control_thread(void* arg) { // Function to control the buzzer
    while (1) {
        pthread_mutex_lock(&lock);
        // Change the condition to activate the buzzer when light_value is less than LIGHT_THRESHOLD
        if (vibration_value > VIBRATION_THRESHOLD && light_value < LIGHT_THRESHOLD) {   
            write_gpio(BUZZER_GPIO_PATH, "1");
            printf("Dryer is done and light is below threshold: Buzzer activated!\n");
        } else { // Deactivate the buzzer otherwise
            write_gpio(BUZZER_GPIO_PATH, "0");
            printf("Buzzer deactivated.\n");
        }
        pthread_mutex_unlock(&lock);
        usleep(100000); // 100 ms
    }
    return NULL;
}

int main() { // Main function
    printf("Starting the dryer alarm system...\n");
    pthread_t threads[4];   // Array of threads
    pthread_mutex_init(&lock, NULL);

    pthread_create(&threads[0], NULL, sensor_reading_thread, NULL); // Create the sensor reading thread
    pthread_create(&threads[1], NULL, led_control_thread, NULL); // Create the LED control thread
    pthread_create(&threads[2], NULL, buzzer_control_thread, NULL); // Create the buzzer control thread
    pthread_create(&threads[3], NULL, email_notification_thread, NULL);  // Create the email notification thread

    for (int i = 0; i < 4; i++) { // Join the threads
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);
    return 0;
}