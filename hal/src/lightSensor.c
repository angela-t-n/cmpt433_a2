#include "hal/lightSensor.h"

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>

// Sensor uses I2C

// Device bus & address
#define I2CDRV_LINUX_BUS "/dev/i2c-1"
#define I2C_DEVICE_ADDRESS 0x48
// Register in TLA2024
#define REG_CONFIGURATION 0x01
#define REG_DATA 0x00
// Configuration reg contents for continuously sampling different channels
#define TLA2024_CHANNEL_CONF_2 0x83E2 // LED recieve




// Allow module to ensure it has been initialized (once!)
static bool is_initialized = false;
static int i2c_file_desc = -1;




// Save the history of light sensor readings
// 100 max readings, 1 reading per ms (1 second == 100 ms)
#define MAX_HISTORY_SIZE 100
static double lightHistory[MAX_HISTORY_SIZE];
static double prevSecHistory[MAX_HISTORY_SIZE];

// the current index to store
static int historyIndex = 0;

// total samples overtime
static long long totalSamples = 0;

#define SMOOTHING_FACT 0.001
static double sampleAverage = 0.0;




/*
Provided functions from I2C Tutorial
==========================================*/

int init_i2c_bus(char *bus, int address)
{
    int i2c_file_desc = open(bus, O_RDWR);
    if (i2c_file_desc == -1)
    {
        printf("I2C DRV: Unable to open bus for read/write (%s)\n", bus);
        perror("Error is:");
        exit(EXIT_FAILURE);
    }
    if (ioctl(i2c_file_desc, I2C_SLAVE, address) == -1)
    {
        perror("Unable to set I2C device to slave address.");
        exit(EXIT_FAILURE);
    }
    return i2c_file_desc;
}

void write_i2c_reg16(int i2c_file_desc, uint8_t reg_addr, uint16_t value)
{
    int tx_size = 1 + sizeof(value);
    uint8_t buff[tx_size];
    buff[0] = reg_addr;
    buff[1] = (value & 0xFF);
    buff[2] = (value & 0xFF00) >> 8;
    int bytes_written = write(i2c_file_desc, buff, tx_size);
    if (bytes_written != tx_size)
    {
        perror("Unable to write i2c register");
        exit(EXIT_FAILURE);
    }
}

uint16_t read_i2c_reg16(int i2c_file_desc, uint8_t reg_addr)
{
    // To read a register, must first write the address
    int bytes_written = write(i2c_file_desc, &reg_addr, sizeof(reg_addr));
    if (bytes_written != sizeof(reg_addr))
    {
        perror("Unable to write i2c register.");
        exit(EXIT_FAILURE);
    }
    // Now read the value and return it
    uint16_t value = 0;
    int bytes_read = read(i2c_file_desc, &value, sizeof(value));
    if (bytes_read != sizeof(value))
    {
        perror("Unable to read i2c register");
        exit(EXIT_FAILURE);
    }
    return value;
}

/*
Helper Function (I made this for A1)
==========================================*/

// converts raw joysticc values
uint16_t convRawRead(uint16_t rawVal)
{
    uint16_t frontHalf, backHalf;

    // front half is the low bytes, ex: 0x1234 would just be 0x34
    // back half is the high bytes, which would be 0x12

    // Mask
    frontHalf = rawVal & 0xff;

    // shift "downwards"
    backHalf = rawVal >> 8;

    // then give the fronthalf some "space" (0x34 would become 0x3400?)
    frontHalf <<= 8;

    // then add in the back half
    // 0x3400 + 0x12 = 0x3412
    frontHalf += backHalf;

    // cool this works, and now lower it to 12 bits so shift over by 4 to make the bottom 4 0
    frontHalf >>= 4;

    return frontHalf;
}




void lightSensor_init(void){

    // Noticed this was in the buttons.c so I decided to include it.
    assert(!is_initialized);

    // I wanted intialize outputs for everything.
    printf("Intializing Light Sensor...\n");

    // start initializing the light sensor
    i2c_file_desc = init_i2c_bus(I2CDRV_LINUX_BUS, I2C_DEVICE_ADDRESS);
    
    is_initialized = true;
}

void lightSensor_cleanup(void){
    assert(is_initialized);
    printf("Stopping Light Sensor...\n");
    
    // simply close the file (only clean-up we got tbh)
    close(i2c_file_desc);
    is_initialized = false;
}



uint16_t lightSensor_readVal(void){
    // Using the numbers for the i2c we already defined, read it.
    // remember: we're using ch 2 for the light reader.
    write_i2c_reg16(i2c_file_desc, REG_CONFIGURATION, TLA2024_CHANNEL_CONF_2);
    
    // read the raw values
    uint16_t raw_read = read_i2c_reg16(i2c_file_desc, REG_DATA);

    // once read, convert the bytes
    return convRawRead(raw_read);
}



void lightSensor_moveCurrentDataToHistory(void){
    // given a piece of data, we should put it into the array.
    // Base case 0: array is full
    // loop back around since it's not a malloc'd arr.
    if(historyIndex == MAX_HISTORY_SIZE){
        // copy it to the previous history
        for(int i = 0 ; i < MAX_HISTORY_SIZE ; i++){
            prevSecHistory[i] = lightHistory[i];
        }

        // reset history index to overwrite values
        historyIndex = 0;
    }

    // chuck the data into the array at that index
    uint16_t currReading = lightSensor_readVal();
    lightHistory[historyIndex++] = currReading;

    // increment the number of logged history
    totalSamples++;

    // now calculate the average
    // exponentially smoothed out btw
    // using formuma found in lecture slides for the smoothing.
    
    // if it's the very first sample, the current average IS the new reading.
    if(totalSamples == 1){
        sampleAverage = currReading;
    }
    else{
        /* "You only need the
        previously calculated average value and the latest sample. It has nothing to do with the
        samples stored in the history."
        
        the formula for this would be: v_n = a * s_n + (1 - a) * v_(n-1)
        where:
        v_n = new average
        v_(n-1) = previous average
        s_n = new val
        a = smoothing factor, which is 0.001 in order to achieve a weight of 99.99% on the prev. avg.
        */
       sampleAverage = SMOOTHING_FACT * currReading + (1 - SMOOTHING_FACT) * sampleAverage;
    }
}

int lightSensor_getHistorySize(void){
    // just return the current history index.
    // that's the "size" of the array.
    return historyIndex;
}

double* lightSensor_getHistory(int *size){
   // make a double arr malloc'd with exactly the history size.
   *size = MAX_HISTORY_SIZE;
   double *historyCopy = malloc(MAX_HISTORY_SIZE *  sizeof(double));

   // if it was unable to malloc:
   if (historyCopy == NULL)
    {
        perror("Unable to allocate memory for history copy. :((((( mega SAD");
        exit(EXIT_FAILURE);
    }

   // perform a deep copy, i wanna do a deep dive into my bed rn im so sleepy it's 4 am send help
   for(int i = 0 ; i < MAX_HISTORY_SIZE ; i++){
        historyCopy[i] = prevSecHistory[i];
   }

   // give it back
   return historyCopy;
}



// Get the average light level (not tied to the history).
double lightSensor_getAverageReading(void){
    // just returns the average already calculated previously
    return sampleAverage;
}

// Get the total number of light level samples taken so far.
long long lightSensor_getNumSamplesTaken(void){
    // just returns the big total
    return totalSamples;
}