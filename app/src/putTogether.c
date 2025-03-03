#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "putTogether.h"

// the board stuff
#include "hal/gpio.h"
#include "hal/btn_statemachine.h"
#include "hal/lightSensor.h"
#include "hal/rotary.h"
#include "hal/lcdDisplay.h"
#include "hal/pwmLed.h"

// my scuffed attempt at networking
#include "udpHandler.h"
#include <string.h>
#include <pthread.h>

#include "sleepHelper.h"
#include "periodTimer.h"

#include <time.h>

// used during the stage of testing every single little module
// just an artifact of my pain and suffering
#define TESTING 0

// Globals
static bool RUN_FLAG = true;
static int dips = 0;

// initializes everything
void putTogether_init(){
    lightSensor_init();
    pwmLed_init();

    rotary_init();
    BtnStateMachine_init();
    lcdDisplay_init();

    Period_init();
}

// cleansup everything
void putTogether_cleanup(){
    printf("\n");

    lightSensor_cleanup();
    pwmLed_cleanup();

    rotary_cleanup();
    BtnStateMachine_cleanup();
    lcdDisplay_cleanup();

    Period_cleanup();

    printf("Exiting...\n");
}



// 1.1 Light Sensor
#if TESTING
void printDoubleArrAndFree(double* arr, const int* size){
    // print out the history for this past 1 ms
    for (int i = 0; i < *size; i++)
    {
        if(i%10 == 0){
            // make a new line, 10 entries per line.
            printf("\n");
        }

        printf("%f ", arr[i]);
    }
    // then demalloc it
    free(arr);
}

void printTestLightSensor(){
    printf("\n\nHistory:\n");
    // read the num and print it out]

    // get the history
    int historySize = 0;
    double *historyArr = lightSensor_getHistory(&historySize);
    printDoubleArrAndFree(historyArr, &historySize);
    
    printf("\n\n");

    // print out the weighted average thing
    double avg = lightSensor_getAverageReading();
    long long total = lightSensor_getNumSamplesTaken();
    printf("Current Average: %f, \t total Samples: %lld\n", avg, total);
}

void testLightSensor()
{
    lightSensor_init();

    int counter = 0;
    while (true)
    {
        lightSensor_moveCurrentDataToHistory();

        // every 1000 ms, get the history
        if (counter == 9999)
        {
            printTestLightSensor();

            // resets
            counter = 0;
        }

        // sleep for 1ms
        counter++;
        milisecondSleep(1);
    }

    lightSensor_cleanup();
}
#endif

// 1.2 is UDP
#if TESTING
void testUDP(){
    udpHandler_init();

    // save the messages
    char msg[1024];

    while(1){
        udpHandler_recieve(msg);

        // if byebye, then break loop
        if(strncmp(msg, "byebye", 6) == 0){
            printf("Closing server, byebye~\n");
            break;
        }

        // echo back the recieved msg
        char replyMsg[] = "Message received! Echoing:";
        udpHandler_reply(replyMsg);
        udpHandler_reply(msg);
    }

    udpHandler_close();
}
#endif




// 1.3 Detect Light Dips
#if TESTING
#endif

// idk, some arbituary number
// idk how to convert it to 0.1V cuz im displaying the raw values
// nvm i figured it out, it's now in voltages
#define LIGHT_DIP_TRESH      0.10
#define LIGHT_DIP_HYSTERESIS 0.03
// that seems a little low but w/e

int putTogether_detectLightDips(){
    // Firstly, get the history
    int arrSize = 0;
    double* lightHistory = lightSensor_getHistory(&arrSize);

    // get the average as well
    double lightAvg = lightSensor_getAverageReading();

    // iterate through and check if there are any dips in the history
    int dipCounter = 0;
    bool dipDetected = false;

    double dipThresh = lightAvg - LIGHT_DIP_TRESH;
    double resetTresh = lightAvg - (LIGHT_DIP_TRESH - LIGHT_DIP_HYSTERESIS);
    for(int i = 0 ; i < arrSize ; i++){
        // check for dip
        if(!dipDetected && lightHistory[i] < dipThresh){
            // update the dip counter
            dipCounter++;

            // set it to true
            dipDetected = true;
        }

        // if there was a previous dip detected, and the values
        // went up past the hysteresis, then allow
        // for another dip finding
        else if(dipDetected && lightHistory[i] > resetTresh){
            dipDetected = false;
        }
    }

    // then, free the history since it was malloc'd
    free(lightHistory);
    
    // return the amount of dips detected in this last history
    return dipCounter;
}

void putTogether_updateLightDips(){
    // add in the value to the static var
    // nvm im stupid, it's not add, i just replace it completely
    dips = putTogether_detectLightDips();
}



static double MAX_ADC_MS = 0.0;

// 1.4 nice terminal output
void putTogether_outputEverySecond(){
    // Line 1:
    // a) # of samples taken this past second
    printf("Samples taken: %d\t", lightSensor_getHistorySize());

    // b) PWM Flash hz
    printf("Flash Hz: %d\t", pwmLed_getHz());

    // c) Avg light levels
    printf("Avg Light Levels: %f\t", lightSensor_getAverageReading());

    // d) Number of light dips in total
    putTogether_updateLightDips();
    printf("Light Dips: %d\t", dips);

    // e) periodTimer.h output
    Period_statistics_t stats;
    Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &stats);

    // print it out with that crazy format
    printf("Smpl ms[%.3f, %.3f] avg %.3f/%d\n", stats.minPeriodInMs, stats.maxPeriodInMs, stats.avgPeriodInMs, stats.numSamples);
    //store the max ms for the lcd
    MAX_ADC_MS = stats.maxPeriodInMs;



    // Line 2:
    // print 10 samples from the past second, evenly spaced
    int historySize = 0;
    double * lightHistory = lightSensor_getHistory(&historySize);

    int increment = 1;
    if(historySize > 10){
        // more than 10 "history" so we can increment it by some floored amount
        increment = historySize / 10;
    }

    // display them
    for(int i = 0 ; i < historySize ; i += increment){
        // print the sample stored here
        printf("%.3f\t", lightHistory[i]);
    }

    // free the history
    free(lightHistory);
    printf("\n");
}



// 1.5 Rotary and PWM
// it is what it is
#if TESTING
void testRotary(){
    rotary_init();

    while(true){
        rotary_doState();
    }

    rotary_cleanup();
}

void testPwmLed(){
    pwmLed_init();

    pwmLed_setFlash(5);

    pwmLed_on();

    // stay on for like 3 seconds
    secondSleep(3);

    // turn it off
    pwmLed_off();

    // clean
    pwmLed_cleanup();
}
#endif

#define STARTING_HZ 10
void putTogether_readRotaryUpdatePWM(){
    // firstly, get the current rotary counter
    int count = rotary_getCounterVal();

    // then, scale that with the starting hz
    int newHZ = count + STARTING_HZ;

    // set the flash
    // this auto puts it to the range and prevents delays
    pwmLed_setFlash(newHZ);
}


// 1.6 LCD
#if TESTING
void testLCD(){
    lcdDisplay_init();

    //char buff[] = "meow meow nnyaa nyaa\nmeow meow nnyaa nyaa\nmeow meow nnyaa nyaa";
    // 21 characters wide
    // 12 characters long
    char buff[] = "123456789012345678901\n2\n3\n4\n5\n6\n7\n8\n9\n10\n1111111111111111111111111 222222222222222223 333333333333 ";
    lcdDisplay_updateScreen(buff);

    // after like 3 seconds, clear the screen
    secondSleep(3);

    lcdDisplay_clearScreen();

    lcdDisplay_cleanup();
}
#endif
void putTogether_updateDisplayEverySecond(){
    // shouldn't need to be THAT big
    const int max = 1024;
    char buff[max];
    int buffOffset = 0;

    // sprintf the messages in
    buffOffset += snprintf(buff + buffOffset, max - buffOffset, "Angela's\n");
    buffOffset += snprintf(buff + buffOffset, max - buffOffset, "Flash @ %d\n", pwmLed_getHz());
    buffOffset += snprintf(buff + buffOffset, max - buffOffset, "Dips @ %d\n", dips);
    //double TODOMS = 0.0;
    buffOffset += snprintf(buff + buffOffset, max - buffOffset, "Max ms @ %.1f\n \n", MAX_ADC_MS);

    // as a bonus, the direction the STUPID stick is spinning
    buffOffset += snprintf(buff + buffOffset, max - buffOffset, "Rotary:\n");
    buffOffset += snprintf(buff + buffOffset, max - buffOffset, "Dir @ %s\n", rotary_getDirection());
    snprintf(buff + buffOffset, max - buffOffset, "Counter @ %d\n", rotary_getCounterVal());


    // put it into the display
    lcdDisplay_updateScreen(buff);
}




// 1.7 use the periodTimer.h/.c provided on the website
// did it, it's simplier than I thought

// end of part 1. Submit as a tar


// 2. is not part of this program
// instead, i gdb debug the noworky.c file
// submit the debugging session text
// take a screenshot of debugging it in vscode


// ok maybe the threads aren't that bad. also yes the ordering
// on this function is wack. I made this thread after the bottom two
void* putTogether_rotaryAndPWM_thread(void* arg){
    // I don't think I actually need the arg
    (void)arg;

    // turn on the pwm
    pwmLed_on();

    // while the run loop is still cooking
    while(RUN_FLAG){
        
        // update the pwm led based on the rotary counter
        // position, btw that rotary is scuffed
        putTogether_readRotaryUpdatePWM();
        rotary_doState();

    }

    // turn off the pwm
    pwmLed_off();

    // and then return the thread
    printf("Returning from Rotary and Pwm Thread...\n");
    return NULL;
}


// The thing that puts it all together in a single module
void* putTogether_lightSensor_thread(void* arg){
    // I don't think I actually need the arg
    (void)arg;

    printf("\nStarting Light Sensor Loop!\n\n");
    
    // in order to determine if a second has passed irl, use a timer thing
    time_t prevTime = time(NULL);

    while(RUN_FLAG){
        // keep doing what it needs to do
        // get the current time it started this loop
        time_t currTime = time(NULL);
        
        // Check to see if a second or more has passed
        if(currTime - prevTime >= 1){
            // perform the actions per second

            // move all the data read into history for this past second.
            lightSensor_moveCurrentDataToHistory();

            // display terminal output
            putTogether_outputEverySecond();

            // display lcd output
            putTogether_updateDisplayEverySecond();

            // update the timer
            prevTime = currTime;
        }

        // get light sample
        lightSensor_readAndStore();

        // mark this as an event
        Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);

        // also update the dips detected
        putTogether_updateLightDips();

        // sleep for 1 ms per sample
        // it already runs way too slow
        //milisecondSleep(1);
    }

    // loop done, tell the thread that
    printf("Returning from Light Sensor Thread...\n");
    return NULL;
}


// needed huge buffer cuz damn that's a lot of numbers
#define MAX_MSG_LEN   8192
char reply[MAX_MSG_LEN];

void replyMsg(char* resv, char* resp){
    // clear out anything left in resp
    memset(resp, 0, MAX_MSG_LEN);

    // user asked for help
    if((strncmp(resv, "help", 4) == 0) || (strncmp(resv, "?", 1) == 0)){
        // make the msg
        char helpMessage[] =
            "[Server] Accepted command examples (Case Sensitive!):\n"
            "count -- get the total number of samples taken.\n"
            "length -- get the number of samples taken in the previously completed second.\n"
            "dips -- get the number of dips in the previously completed second.\n"
            "history -- get all the samples in the previously completed second.\n"
            "stop -- cause the server program to end.\n"
            "<enter> -- repeat last command.\n";

            // copy the string over
            strcpy(resp, helpMessage);
        
    }

    // user asked for count
    else if((strncmp(resv, "count", 5) == 0)){
        // use sprintf to "print" the message back
        snprintf(resp, MAX_MSG_LEN, "[Server] Samples taken in total: %lld\n", lightSensor_getNumSamplesTaken());
    }

    // user asked for history length
    else if((strncmp(resv, "length", 6) == 0)){
        // use sprintf to "print" the message back
        snprintf(resp, MAX_MSG_LEN, "[Server] Samples taken in the last second %d\n", lightSensor_getHistorySize());
    }

    // user asked for dips
    else if((strncmp(resv, "dips", 4) == 0)){
        // use sprintf to "print" the message back
        snprintf(resp, MAX_MSG_LEN, "[Server] Dips Detected %d\n", dips);
    }

    // user asked for history
    else if((strncmp(resv, "history", 7) == 0)){
        // get the history
        int size = 0;
        double* hist = lightSensor_getHistory(&size);

        // have to iterate through the entire history array
        // store it into a buffer
        char buffer[MAX_MSG_LEN];

        // im stupid it was just overwriting itself
        int bufferOffset = 0;

        bufferOffset += snprintf(buffer + bufferOffset, MAX_MSG_LEN - bufferOffset, "[System] History in the past second:\n");

        for(int i = 0 ; i < size ; i++){
            // if mod by 10 possible, then we've hit the 10th number
            if(i%10 == 0 && i != 0){
                // make a new line, 10 entries per line.
                bufferOffset += snprintf(buffer + bufferOffset, MAX_MSG_LEN - bufferOffset, "\n");
            }

            // chuck this current number into the buffer
            bufferOffset += snprintf(buffer + bufferOffset, MAX_MSG_LEN - bufferOffset, "%0.3f, ", hist[i]);
        }

        // done typing all the numbers, add a final \n
        snprintf(buffer + bufferOffset, MAX_MSG_LEN - bufferOffset, "\n");

        // buffer should be built now, just copy it over
        strcpy(resp, buffer);

        // free the memory
        free(hist);
    }

    // something useless
    else{
        snprintf(resp, MAX_MSG_LEN, "[Server] Unknown Command!\n");
    }
}

// I'm so scared of using threads but i have to aaahh im like allergic to them i swear
void* putTogether_udpHandler_thread(void* arg){
    // I don't think I actually need the arg
    (void)arg;

    udpHandler_init();

    // save the messages
    char msg[MAX_MSG_LEN];

    while(1){
        udpHandler_recieve(msg);

        // if exit, then break loop and end the program
        if(strncmp(msg, "stop", 4) == 0){
            printf("Closing server, byebye~\n");
            udpHandler_reply("[Server] Program Exiting!\n\n");

            RUN_FLAG = false;
            break;
        }

        // if enter was pressed, just return the previous msg stored
        else if(strncmp(msg, "\n", 1) == 0){
            udpHandler_reply(reply);
        }

        // otherwise
        else{
            // Handle the other reply messages
            replyMsg(msg, reply);

            //printf("Replying with: %s\n", reply);

            // reply back to them
            udpHandler_reply(reply);
        }

    }

    udpHandler_close();

    // loop done, tell the thread that
    printf("Returning from UDP Thread...\n");
    return NULL;
}










// so ummm, it's kinda impossible to have both the udp stuff running AND the silly stupid light sensor
// im dying it's 7 am
void putTogether_runLoop(){
    // create the threads for the light sensor bs and the udp
    pthread_t lightThread, udpThread, rotaryPWMThread;

    // start the threads
    pthread_create(&lightThread, NULL, putTogether_lightSensor_thread, NULL);
    pthread_create(&udpThread, NULL, putTogether_udpHandler_thread, NULL);
    pthread_create(&rotaryPWMThread, NULL, putTogether_rotaryAndPWM_thread, NULL);

    // wait for both threads to finish before going byebye
    pthread_join(udpThread, NULL);
    pthread_join(lightThread, NULL);
    
    // gets stuck on doState so u needa spin it
    printf("JIggle the joystick to free it from doState please!\n");
    pthread_join(rotaryPWMThread, NULL);
}