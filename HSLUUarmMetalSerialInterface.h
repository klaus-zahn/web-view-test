/* Copyright 2016 Hochschule Luzern - Informatik */

#ifndef HSLUUARMMETALSERIALINTERFACE_H
#define HSLUUARMMETALSERIALINTERFACE_H

/**
 * Definitions for the HSLU UArmMetal serial interface.
 * @author Peter Sollberger (peter.sollberger@hslu.ch)
 */

// Size definitions
const int HEADER_SIZE = 3;
const int MAX_MESSAGE_SIZE = 10;

// Alert byte definitions
const char HEADER_ALERT_BYTE = '\xAA';
const char HEADER_RESPONSE_BYTE = '\xBB';

// Command byte definitions
typedef enum uarmMetalCmd_ {
    ATTACH_DETACH = 0x01,
    IS_ATTACHED,
    WRITE_SERVO_ANGLE,
    READ_SERVO_ANGLE,
    MOVE_TO,
    GET_POSITION,
    PICK,
    PUMP_ON_OFF,
    STOP
} uarmMetalCmd_t;

// Response byte definitions
typedef enum uarmMetalResponse_ {
    ATTACHED_STATE = 0x01,
    SERVO_ANGLE,
    POSITION
} uarmMetalResponse_t;

// Servo number definitions
typedef enum servo_ {
    SERVO_ROT,
    SERVO_LEFT,
    SERVO_RIGHT,
    SERVO_HAND
} servo_t;

#endif /* HSLUUARMMETALSERIALINTERFACE_H */
