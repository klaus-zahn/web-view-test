/* Copyright 2016 Hochschule Luzern - Informatik */

#include <iostream>

#ifndef _WIN32
#include <unistd.h>  // UNIX standard function definitions
#include <fcntl.h>   // File control definitions
#endif

#include "HSLUUarmInterface.h"

using namespace std;

/**
 * Time to wait for a response for getPosition and isAttached,
 */
#define RESPONSE_WAIT_TIME 500

/**
 * Receive state machine.
 */
typedef enum receiveState_ {
    WAIT_ALERT = 1, WAIT_RESPONSE, WAIT_LEN, WAIT_DATA
} receiveState_t;

HSLUUarmInterface::~HSLUUarmInterface(void) {
    if (fd) {
        closeAll();
    }
}

/**
 * Open COM port and start receive thread.
 */
bool HSLUUarmInterface::initialize(const char* port, speed_t baudrate) {
    bool rc = false;

#ifdef _WIN32
    string serialPort("\\\\.\\");
    serialPort += port;
    //e.g. "\\\\.\\COM22"
    fd = CreateFile(
            (const char*) serialPort.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            0,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            0
            );

    if (fd != INVALID_HANDLE_VALUE) {
        DCB dataControlBlock = {0};
        GetCommState(fd, &dataControlBlock);
        dataControlBlock.BaudRate = 9600;
        dataControlBlock.ByteSize = 8;
        dataControlBlock.Parity = NOPARITY;
        dataControlBlock.StopBits = ONESTOPBIT;

        dataControlBlock.fBinary = true;
        dataControlBlock.fNull = false; 
        dataControlBlock.fDtrControl = DTR_CONTROL_DISABLE;
        dataControlBlock.fRtsControl = RTS_CONTROL_DISABLE;
        dataControlBlock.fOutxCtsFlow = FALSE;
        dataControlBlock.fOutxDsrFlow = FALSE;
        dataControlBlock.fDsrSensitivity = FALSE;
        dataControlBlock.fAbortOnError = FALSE;

        if (SetCommState(fd, &dataControlBlock)) {
            COMMTIMEOUTS timeOuts;
            timeOuts.ReadIntervalTimeout = 20;
            timeOuts.ReadTotalTimeoutMultiplier = 0;
            timeOuts.ReadTotalTimeoutConstant = 20;
            timeOuts.WriteTotalTimeoutMultiplier = 0;
            timeOuts.WriteTotalTimeoutConstant = 0;
            SetCommTimeouts(fd, &timeOuts);
            rc = true;
        } else {
            rc = false;
        }
#else
    struct termios options;

    fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd != -1) {
        // open successfull
        int i, f;

        // Force read call to block if no data available
        f = fcntl(fd, F_GETFL, 0);
        f &= ~O_NONBLOCK;
        fcntl(fd, F_SETFL, f);

        // Get the current options for the port...
        tcgetattr(fd, &options);
        // ... and set them to the desired values        
        cfsetispeed(&options, baudrate); // baud rates to 9600...
        cfsetospeed(&options, baudrate);
        options.c_cflag &= ~PARENB; // no parity (8N1)
        options.c_cflag &= ~CSTOPB;
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;
        options.c_cflag &= ~CRTSCTS; // disable hardware flow control        
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // raw input        
        options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL); // disable software flow control        
        options.c_oflag &= ~(OPOST | OCRNL); // raw output

        // clear all control characters
        for (i = 0; i < NCCS; i++) {
            options.c_cc[i] = 0;
        }
        // Set byte times
        options.c_cc[VMIN] = 1;
        options.c_cc[VTIME] = 0;

        // Set the new options for the port
        tcsetattr(fd, TCSAFLUSH, &options);

        // Flush to put settings to work
        tcflush(fd, TCIOFLUSH);
#endif
        // Start thread
        receiveThread = thread(&HSLUUarmInterface::runReceive, this);
        this_thread::yield();  // Wait for thread to start
        rc = true;
    } else {
        fd = 0;
        cout << "Unable to initialize port " << port << endl;
    }

    return rc;
}

/**
 * Close COM port and stop receive thread.
 */
void HSLUUarmInterface::closeAll(void) {
    // Close port
    finished = 1;
    if (fd) {
#ifdef _WIN32
        CloseHandle(fd);
#else
        close(fd);
#endif
        // Wait for receive thread to terminate
        receiveThread.join();
    }
    cout << endl << "All closed." << endl;
    fd = 0;
}

/**
 * Output received message.
 */
void HSLUUarmInterface::interpretMessage(char* msg, int len) {
    int16_t x, y, z;

    // Get lock
    lock_guard<mutex> lk(mux);

    switch (msg[1]) {
        case ATTACHED_STATE:
            attached = msg[4] != 0;
            break;
        case SERVO_ANGLE:
            angle = (msg[4] * 128 + msg[5]);
            break;
        case POSITION:
            x = (msg[3] * 128 + msg[4]);
            y = (msg[5] * 128 + msg[6]);
            z = (msg[7] * 128 + msg[8]);
            pos = Position(x, y, z);
            break;
        default:
            break;
    }
    cv.notify_all();
}

/**
 * Thread function for COM port receiving.
 * Writes received characters on the screen.
 */
void HSLUUarmInterface::runReceive(void) {
    char buffer[HEADER_SIZE + MAX_MESSAGE_SIZE];
    int bufferBegin = 0;
    receiveState_t state = WAIT_ALERT;
#ifdef _WIN32
    DWORD bytesRead;
    DWORD bytesToRead = 1;
#else
    int bytesRead;
    int bytesToRead = 1;
#endif

    do {
#ifdef _WIN32
        ReadFile(fd, buffer + bufferBegin, bytesToRead, &bytesRead, NULL);
#else
        bytesRead = read(fd, buffer + bufferBegin, bytesToRead);
#endif
        // Read call blocks until a byte arrives
        if ((bytesRead > 0)) {
            switch (state) {
                case WAIT_ALERT:
                    if ((buffer[0] == ((char) HEADER_RESPONSE_BYTE))) {
                        bufferBegin = 1;
                        bytesToRead = 1;
                        state = WAIT_RESPONSE;
                    } else {
                        // Maybe we receive somthing from Serial.print(...) calls.
                        cout << buffer[0];
                        cout.flush();
                        bufferBegin = 0;
                        bytesToRead = 1;
                        state = WAIT_ALERT;
                    }
                    break;
                case WAIT_RESPONSE:
                    if (buffer[1] > 0) { // A valid response
                        bufferBegin = 2;
                        bytesToRead = 1;
                        state = WAIT_LEN;
                    } else {
                        bufferBegin = 0;
                        bytesToRead = 1;
                        state = WAIT_ALERT;
                    }
                    break;
                case WAIT_LEN:
                    if (buffer[2] == 0) { // Message is complete
                        interpretMessage(buffer, HEADER_SIZE);
                        bufferBegin = 0;
                        bytesToRead = 1;
                        state = WAIT_ALERT;
                    } else if ((buffer[2] > 0) && (buffer[2] <= MAX_MESSAGE_SIZE)) {
                        bufferBegin = 3;
                        bytesToRead = buffer[2];
                        state = WAIT_DATA;
                    } else {
                        bufferBegin = 0;
                        bytesToRead = 1;
                        state = WAIT_ALERT;
                    }
                    break;
                case WAIT_DATA:
                    if (bytesRead < bytesToRead) {
                        bufferBegin += bytesRead;
                        bytesToRead -= bytesRead;
                    } else {
                        interpretMessage(buffer, buffer[2]);
                        bufferBegin = 0;
                        bytesToRead = 1;
                        state = WAIT_ALERT;
                    }
                    break;
                default:
                    bufferBegin = 0;
                    bytesToRead = 1;
                    state = WAIT_ALERT;
                    break;
            }
        } else {
            // read should not return until at least one byte arrives 
            // or the port is closed
            // cout << endl << "Zero bytes received..." << endl;
        }
    } while (!finished);

    cout << "Leaving receive thread..." << endl;
}

void HSLUUarmInterface::sendMessage(const char* buffer, int len) {
    if (fd) {
#ifdef _WIN32
        unsigned long numberBytesWritten;
        WriteFile(fd, buffer, len, &numberBytesWritten, NULL);
		FlushFileBuffers(fd);
#else
        ssize_t ret = write(fd, buffer, len);
        if(ret != len) {
            cout << "problem writing values" << endl;
        }
        tcflush(fd, TCOFLUSH);
#endif
    }
}

void HSLUUarmInterface::askForAttachState(servo_t servo) {
    char buffer[HEADER_SIZE + MAX_MESSAGE_SIZE];

    buffer[0] = HEADER_ALERT_BYTE;
    buffer[1] = IS_ATTACHED;
    buffer[2] = 1;
    buffer[3] = servo;

    sendMessage(buffer, 4);
}

void HSLUUarmInterface::askForAngle(servo_t servo) {
    char buffer[HEADER_SIZE + MAX_MESSAGE_SIZE];

    buffer[0] = HEADER_ALERT_BYTE;
    buffer[1] = READ_SERVO_ANGLE;
    buffer[2] = 1;
    buffer[3] = servo;

    sendMessage(buffer, 4);
}

void HSLUUarmInterface::askForPosition(void) {
    char buffer[HEADER_SIZE + MAX_MESSAGE_SIZE];
    buffer[0] = HEADER_ALERT_BYTE;
    buffer[1] = GET_POSITION;
    buffer[2] = 0;

    sendMessage(buffer, 3);
}

void HSLUUarmInterface::attachDetachAllServos(bool attach) {
    char buffer[HEADER_SIZE + MAX_MESSAGE_SIZE];

    buffer[0] = HEADER_ALERT_BYTE;
    buffer[1] = ATTACH_DETACH;
    buffer[2] = 2;
    buffer[3] = '\xFF'; // All servor
    buffer[4] = attach;

    sendMessage(buffer, 5);
}

void HSLUUarmInterface::attachDetach(servo_t servo, bool attach) {
    char buffer[HEADER_SIZE + MAX_MESSAGE_SIZE];

    buffer[0] = HEADER_ALERT_BYTE;
    buffer[1] = ATTACH_DETACH;
    buffer[2] = 2;
    buffer[3] = servo;
    buffer[4] = attach;

    sendMessage(buffer, 5);
}

bool HSLUUarmInterface::isAttached(servo_t servo) {
    unique_lock<mutex> lk(mux);

    attached = false; // Default value
    askForAttachState(servo);
    cv.wait_for(lk, chrono::milliseconds(RESPONSE_WAIT_TIME));

    return attached;
}

void HSLUUarmInterface::writeAngle(servo_t servo, int16_t angle, uint8_t speed) {
    char buffer[HEADER_SIZE + MAX_MESSAGE_SIZE];

    buffer[0] = HEADER_ALERT_BYTE;
    buffer[1] = WRITE_SERVO_ANGLE;
    buffer[2] = 4;
    buffer[3] = servo;
    buffer[4] = angle / 128;
    buffer[5] = angle % 128;
    buffer[6] = speed;

    sendMessage(buffer, 7);
}

int16_t HSLUUarmInterface::readAngle(servo_t servo) {
    unique_lock<mutex> lk(mux);

    angle = -1800; // Default value to indicate failure
    askForAngle(servo);
    cv.wait_for(lk, chrono::milliseconds(RESPONSE_WAIT_TIME));

    return angle;
}

void HSLUUarmInterface::moveTo(int16_t x, int16_t y, int16_t z, uint8_t speed) {
    char buffer[HEADER_SIZE + MAX_MESSAGE_SIZE];

    buffer[0] = HEADER_ALERT_BYTE;
    buffer[1] = MOVE_TO;
    buffer[2] = 7;
    buffer[3] = x / 128;
    buffer[4] = x % 128;
    buffer[5] = y / 128;
    buffer[6] = y % 128;
    buffer[7] = z / 128;
    buffer[8] = z % 128;
    buffer[9] = speed;

    sendMessage(buffer, 10);
}

Position HSLUUarmInterface::getPosition(void) {
    unique_lock<mutex> lk(mux);

    pos = Position(0, 0, 0); // Default value for invalid position
    askForPosition();
    cv.wait_for(lk, chrono::milliseconds(2000));

    return pos;
}

void HSLUUarmInterface::pick(uint16_t distance) {
    char buffer[HEADER_SIZE + MAX_MESSAGE_SIZE];

    buffer[0] = HEADER_ALERT_BYTE;
    buffer[1] = PICK;
    buffer[2] = 2;
    buffer[3] = distance / 128;
    buffer[4] = distance % 128;

    sendMessage(buffer, 5);
}

void HSLUUarmInterface::punp(bool on) {
    char buffer[HEADER_SIZE + MAX_MESSAGE_SIZE];

    buffer[0] = HEADER_ALERT_BYTE;
    buffer[1] = PUMP_ON_OFF;
    buffer[2] = 1;
    buffer[3] = on;

    sendMessage(buffer, 4);

}

void HSLUUarmInterface::stop(void) {
    char buffer[HEADER_SIZE + MAX_MESSAGE_SIZE];
    buffer[0] = HEADER_ALERT_BYTE;
    buffer[1] = STOP;
    buffer[2] = 0;

    sendMessage(buffer, 3);
}
