#ifndef ROBOTCONTROLLER_H
#define ROBOTCONTROLLER_H

#include <string>
#include "Position.h"
#include "HSLUUarmInterface.h"

//version conflict with g++ between raspi and toolchain
//#define USE_REGEX

typedef enum robotPosition_ {
    PARKING_1 = 0, PARKING_2, LEFT_STORE_1, LEFT_STORE_2, LEFT_STORE_3, LEFT_STORE_4, LEFT_STORE_5,
    RIGHT_STORE_1, RIGHT_STORE_2, RIGHT_STORE_3, RIGHT_STORE_4, RIGHT_STORE_5, CELL_A1, CELL_A2,
    CELL_A3, CELL_B1, CELL_B2, CELL_B3, CELL_C1, CELL_C2, CELL_C3, NUM_POSITIONS
} robotPosition_t;


// Singelton instance of robot controller class
class RobotController {
public:
    // All calls through this instance as constructor is private
    static RobotController* getInstance();

    // Initialize everything.
    int init(const char* port);
    // Clean up.
    void exit();
    //is it connected
    bool isConnected() {return connected;}

    // Move to given position; only default values are allowed.
    int move2Pos(robotPosition_t posIndex);
    // Set robot head down by DEFAULT_LIFT_STEPS.
    int putRobotDown();
    // Move robot head up by DEFAULT_LIFT_STEPS.
    int liftRobotUp();
    // Move robot head down by DEFAULT_LIFT_STEPS and activate pump and lift again.
    int pick();
    // Move robot head down by DEFAULT_LIFT_STEPS and stop pump and lift again.
    int place();

private:
    // Make constructor private
    RobotController(void);
    ~RobotController(void);
    
    //is it connected
    bool connected;

    // Sets the default positions by reading from file (or using default values)
    void setDefaultPositions();

    // Read default positions from file 'Positions.txt'. 
    // If file is not available, create it.
    void loadFromFile();

    // Move to an arbitrary position (from current position).
    int move2Pos(const Position& pos);

    // The singleton instance of the controller.
    static RobotController* pInstance;

    // The default positions.
    Position defaultPositions[NUM_POSITIONS];

    // The current position.
    Position currentPos;

    // The serial interface to the robot
    HSLUUarmInterface uarmInterface;
};

#endif //ROBOTCONTROLLER_H
