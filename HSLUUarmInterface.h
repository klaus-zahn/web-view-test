/* Copyright 2016 Hochschule Luzern - Informatik */
#ifndef HSLUUARMINTERFACE_H
#define HSLUUARMINTERFACE_H

#include <thread>
#include <mutex>
#include <condition_variable>

#ifdef _WIN32
#include <Windows.h>
typedef DWORD speed_t;
#else
#include <termios.h> // POSIX terminal control definitions, for speed_t
#endif
#include "HSLUUarmMetalSerialInterface.h"
#include "Position.h"

/**
 * UART interface to the UArmMetal robot.
 * @author Peter Sollberger (peter.sollberger@hslu.ch)
 */
class HSLUUarmInterface {
public:

    HSLUUarmInterface(void) : finished(false), fd(0), pos(0, 0, 0), angle(-180), attached(false) {
    }

    /**
     * Close connection and clean up.
     */
    virtual ~HSLUUarmInterface(void);

    /**
     * Open and initalizes the COM port with the give baudrate.
     * Sets the configuration to: 8 data bits, no parity, 1 stop bit.
     * Starts the receive thread which prints received answers to the terminal.
     * @param port     COM port (e.g. /dev/ttyS3)
     * @param baudrate Baudrate out of the bundle B9600, B19200, B38400, B57600, B76800, BB1152000
     * @return True on success.
     */
    bool initialize(const char* port, speed_t baudrate);

    /**
     * Close port and stop receive thread.
     */
    void closeAll(void);

    /**
     * Attach all servos.
     */
    void attachDetachAllServos(bool attach);

    /**
     * Attach one servos.
     */
    void attachDetach(servo_t servo, bool attach);
    /**
     * Returns true if servo is attached.
     */
    bool isAttached(servo_t servo);

    /**
     * Move a servo to the given angel (in 1/10 degree) with the defined speed.
     */
    void writeAngle(servo_t servo, int16_t angle, uint8_t speed);

    /**
     * Read current angle of a servo. Returns the value from the tacho, don't 
     * matter wheather attached or not.
     * @return Angle in 1/10 degree or -180 on read failure.
     */
    int16_t readAngle(servo_t servo);

    /**
     * Move arm to the given position (in mm) with the defined speed.
     */
    void moveTo(int16_t x, int16_t y, int16_t z, uint8_t speed);

    /**
     * Read current position. Returns the values from the tacho, don't 
     * matter wheather attached or not.
     * @return Positin in mm.
     */
    Position getPosition(void);

    /**
     * Move arm in z direction the given distance (in mm) and stop when contact.
     */
    void pick(uint16_t distance);

    /**
     * Turn pump on or off.
     */
    void punp(bool on);

    /**
     * Stop all servo movement and turn off pump.
     */
    void stop(void);

private:
    /**
     * Ask for actual attach state by sending message.
     */
    void askForAttachState(servo_t servo);

    /**
     * Ask for the actual angle of a servo by sending message.
     */
    void askForAngle(servo_t servo);

    /**
     * Ask for the actual position by sending message.
     */
    void askForPosition(void);

    /**
     * Sends the passsed message over UART.
     */
    void sendMessage(const char* buffer, int len);

    /**
     * The receive methode that runs in a thread.
     */
    void runReceive(void);

    /**
     * Variable to finish thread.
     */
    volatile bool finished;

    /**
     * COM port handle.
     */
#ifdef _WIN32
    HANDLE fd;
#else
    int fd;
#endif

    /** 
     * Last read position from robot in mm.
     * Value 0/0/0 means invalid position.
     */
    Position pos;

    /**
     * Last read servo angle from robot in 1/10 degree.
     * -180 means invalid value.
     */
    int16_t angle;

    /**
     * Last read servo attach state of robot.
     * False is default value.
     */
    bool attached;

    /**
     * Interpret received messages, store received answer and notify calles.
     */
    void interpretMessage(char* msg, int len);

    /**
     * Thread stuff.
     */
    std::thread receiveThread;
    std::mutex mux;
    std::condition_variable cv;
};
#endif /* HSLUUARMINTERFACE_H */
