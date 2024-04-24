# Dryer Alarm System

## Overview
This Dryer Alarm System is designed to monitor a clothes dryer using a BeagleBone Black (BBB). It detects whether the dryer is running based on vibration readings and monitors ambient light levels. The system uses this data to control LEDs, activate a buzzer under specific conditions, and send an email notification if the dryer remains in a "done" state for more than 30 seconds.

## Hardware Requirements
- BeagleBone Black (BBB)
- Vibration Sensor (e.g., Piezo electric sensor)
- Light Sensor (e.g., Photocell)
- Buzzer
- Red LED
- Green LED
- Resistors (for LEDs and sensors as needed)
- Connecting wires
- Breadboard

## Hardware Setup

### Sensor and Output Connections
1. **Vibration Sensor:**
   - Positive pin to P9_39 (AIN0) for analog input.
   - Negative pin to ground on the BBB.
2. **Light Sensor:**
   - Positive pin to P9_40 (AIN1) for analog input.
   - Negative pin to ground on the BBB.
3. **Red LED:**
   - Anode (longer leg) to GPIO 66.
   - Cathode (shorter leg) through a 330Ω resistor to ground.
4. **Green LED:**
   - Anode to GPIO 67.
   - Cathode through a 330Ω resistor to ground.
5. **Buzzer:**
   - One pin to GPIO 68.
   - Other pin to ground.

### General Connections
- Connect all ground connections to one of the GND pins on the BBB.
- Ensure that each component is securely connected on the breadboard and that the BBB pins are correctly mapped.

## Software Setup

### Environment Setup
- Ensure that the BBB is running a compatible version of Debian or another Linux distribution.
- Update the system: `sudo apt-get update && sudo apt-get upgrade`
- Install required packages: `sudo apt-get install build-essential mailutils`

### Email Configuration
Configure postfix to send emails as previously detailed in the email setup instructions:
- Open Postfix configuration: `sudo nano /etc/postfix/main.cf`
- Add or modify the following lines at the end of the file: 
`relayhost = [smtp.gmail.com]:587`
`smtp_use_tls = yes`
`smtp_sasl_auth_enable = yes`
`smtp_sasl_security_options = noanonymous`
`smtp_sasl_password_maps = hash:/etc/postfix/sasl_passwd`
`smtp_tls_CAfile = /etc/ssl/certs/ca-certificates.crt`

- Create and edit the password file:
- `sudo nano /etc/postfix/sasl_passwd`
- Add the line: `[smtp.gmail.com]:587 your-email@gmail.com:your-password`
- Secure the password file and apply changes:
`sudo postmap /etc/postfix/sasl_passwd`
- Restart the postfix:
`sudo systemctl restart postfix`

### Compiling the Program
- Navigate to the directory containing the source code.
- Compile the code: `gcc -o dryer_alarm dryer_alarm.c -lpthread`
- Run the program: `sudo ./dryer_alarm`

## Operation
- The system continuously reads the vibration and light sensor values.
- The Red LED lights up when the dryer is running (based on vibration threshold).
- The Green LED lights up when the dryer is considered done.
- The buzzer activates if the light value is below a set threshold when the dryer is done.
- If the dryer remains in the "done" state for more than 30 seconds, an email notification is sent.

## Troubleshooting
- Check connections on the breadboard if LEDs or the buzzer are not responding.
- Ensure the email system is correctly configured if notifications are not being received.
- Review system logs (`/var/log/syslog` and `/var/log/mail.log`) for potential errors related to GPIO or email sending.
