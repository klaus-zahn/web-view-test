#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include <array>

#ifdef USE_REGEX
        #include <regex>
#endif

#include "RobotController.h"

using namespace std;

/**
 * Number of tries to reach a position.
 */
#define MAX_TRIES 10

const uint8_t DEFAULT_SPEED = 40;
const uint16_t DEFAULT_HEIGHT = 60;
const uint16_t DEFAULT_LOWER = 30; 
const uint16_t DEFAULT_PICK = 40;
const Position LIFT_UP(0, 0, DEFAULT_HEIGHT);

RobotController* RobotController::pInstance = NULL;

RobotController* RobotController::getInstance() {
    if (pInstance == NULL) {
        pInstance = new RobotController();
    }
    return pInstance;
}

RobotController::RobotController(void) : connected(false), uarmInterface() {
}

RobotController::~RobotController(void) {
}

int RobotController::init(const char* port) {
    int rc = -1;

    if (uarmInterface.initialize(port, 9600)) {
        currentPos = uarmInterface.getPosition();

        uarmInterface.attachDetachAllServos(true);
        this_thread::sleep_for(chrono::milliseconds(100));

        setDefaultPositions();
        loadFromFile();
        rc = move2Pos(defaultPositions[PARKING_1]);
        
        connected = true;
    } else {
        //write default positions to files (for testing)
        setDefaultPositions();
        loadFromFile();
    }
    return rc;
}

void RobotController::loadFromFile() {
	fstream input("../Positions.txt", fstream::in);
	if (input.is_open()) {
		cout << "Reading coordinates from file 'Positions.txt'" << endl;

#ifdef USE_REGEX
		regex words_regex("[^\\s]+");
		for (array<char, 80> line; input.getline(&line[0], 80);) {
			string s(line.data());

			auto words_begin = sregex_iterator(s.begin(), s.end(), words_regex);
			auto words_end = sregex_iterator();
			if (distance(words_begin, words_end) == 3) {
				sregex_iterator i = words_begin;
				string pos = (*i++).str();
				int xval = stoi((*i++).str());
				int yval = stoi((*i++).str());
#else
                for (array<char, 80> line; input.getline(&line[0], 80);) {
                        string s(line.data());

                        size_t p0 = s.find(":");
                        size_t p1 = s.find(" ", p0);

                        if(p0 != string::npos && p1 == p0 +1) {
                                string pos = s.substr(0, 3);
                                size_t p2 = s.find(" ", p1+1);

                                int xval = stoi(s.substr(p1, p2 - p1));
                                int yval = stoi(s.substr(p2));
#endif

				if (pos == "P1:") {
					defaultPositions[PARKING_1] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "P2:") {
					defaultPositions[PARKING_2] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "L1:") {
					defaultPositions[LEFT_STORE_1] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "L2:") {
					defaultPositions[LEFT_STORE_2] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "L3:") {
					defaultPositions[LEFT_STORE_3] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "L4:") {
					defaultPositions[LEFT_STORE_4] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "L5:") {
					defaultPositions[LEFT_STORE_5] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "R1:") {
					defaultPositions[RIGHT_STORE_1] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "R2:") {
					defaultPositions[RIGHT_STORE_2] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "R3:") {
					defaultPositions[RIGHT_STORE_3] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "R4:") {
					defaultPositions[RIGHT_STORE_4] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "R5:") {
					defaultPositions[RIGHT_STORE_5] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "A1:") {
					defaultPositions[CELL_A1] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "A2:") {
					defaultPositions[CELL_A2] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "A3:") {
					defaultPositions[CELL_A3] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "B1:") {
					defaultPositions[CELL_B1] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "B2:") {
					defaultPositions[CELL_B2] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "B3:") {
					defaultPositions[CELL_B3] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "C1:") {
					defaultPositions[CELL_C1] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "C2:") {
					defaultPositions[CELL_C2] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
				else if (pos == "C3:") {
					defaultPositions[CELL_C3] = Position(xval, yval, DEFAULT_HEIGHT);
					cout << "New position for " << pos << " " << xval << "/" << yval << endl;
				}
			}
		}
	}
	else {
		cout << "Writing initial file 'Positions.txt'" << endl;

		fstream output("../Positions.txt", fstream::out);
		output << "This file contains the x/y coordinates in mm of the TicTacToe playground." << endl;
		output << "Parking positions" << endl;
		output << "P1: " << defaultPositions[PARKING_1].getX() << " " << defaultPositions[PARKING_1].getY() << endl;
		output << "P2: " << defaultPositions[PARKING_2].getX() << " " << defaultPositions[PARKING_2].getY() << endl;
		output << "Store positions" << endl;
		output << "L1: " << defaultPositions[LEFT_STORE_1].getX() << " " << defaultPositions[LEFT_STORE_1].getY() << endl;
		output << "L2: " << defaultPositions[LEFT_STORE_2].getX() << " " << defaultPositions[LEFT_STORE_2].getY() << endl;
		output << "L3: " << defaultPositions[LEFT_STORE_3].getX() << " " << defaultPositions[LEFT_STORE_3].getY() << endl;
		output << "L4: " << defaultPositions[LEFT_STORE_4].getX() << " " << defaultPositions[LEFT_STORE_4].getY() << endl;
		output << "L5: " << defaultPositions[LEFT_STORE_5].getX() << " " << defaultPositions[LEFT_STORE_5].getY() << endl;
		output << "R1: " << defaultPositions[RIGHT_STORE_1].getX() << " " << defaultPositions[RIGHT_STORE_1].getY() << endl;
		output << "R2: " << defaultPositions[RIGHT_STORE_2].getX() << " " << defaultPositions[RIGHT_STORE_2].getY() << endl;
		output << "R3: " << defaultPositions[RIGHT_STORE_3].getX() << " " << defaultPositions[RIGHT_STORE_3].getY() << endl;
		output << "R4: " << defaultPositions[RIGHT_STORE_4].getX() << " " << defaultPositions[RIGHT_STORE_4].getY() << endl;
		output << "R5: " << defaultPositions[RIGHT_STORE_5].getX() << " " << defaultPositions[RIGHT_STORE_5].getY() << endl;
		output << "Field positions" << endl;
		output << "A1: " << defaultPositions[CELL_A1].getX() << " " << defaultPositions[CELL_A1].getY() << endl;
		output << "A2: " << defaultPositions[CELL_A2].getX() << " " << defaultPositions[CELL_A2].getY() << endl;
		output << "A3: " << defaultPositions[CELL_A3].getX() << " " << defaultPositions[CELL_A3].getY() << endl;
		output << "B1: " << defaultPositions[CELL_B1].getX() << " " << defaultPositions[CELL_B1].getY() << endl;
		output << "B2: " << defaultPositions[CELL_B2].getX() << " " << defaultPositions[CELL_B2].getY() << endl;
		output << "B3: " << defaultPositions[CELL_B3].getX() << " " << defaultPositions[CELL_B3].getY() << endl;
		output << "C1: " << defaultPositions[CELL_C1].getX() << " " << defaultPositions[CELL_C1].getY() << endl;
		output << "C2: " << defaultPositions[CELL_C2].getX() << " " << defaultPositions[CELL_C2].getY() << endl;
		output << "C3: " << defaultPositions[CELL_C3].getX() << " " << defaultPositions[CELL_C3].getY() << endl;
		output.close();
	}

}

void RobotController::exit() {
    move2Pos(defaultPositions[PARKING_1]);

    uarmInterface.attachDetachAllServos(false);
    uarmInterface.closeAll();
    connected = false;
}

void RobotController::setDefaultPositions() {
    //parking positions
    defaultPositions[PARKING_1] = Position(-131, 39, 60);   // R0
    defaultPositions[PARKING_2] = Position(131, 41, 60);   // L0
                                                                                                               //left store
    defaultPositions[LEFT_STORE_1] = Position(136, 112, DEFAULT_HEIGHT);
    defaultPositions[LEFT_STORE_2] = Position(120, 160, DEFAULT_HEIGHT);
    defaultPositions[LEFT_STORE_3] = Position(120, 212, DEFAULT_HEIGHT);
    defaultPositions[LEFT_STORE_4] = Position(122, 257, DEFAULT_HEIGHT);
    defaultPositions[LEFT_STORE_5] = Position(117, 305, DEFAULT_HEIGHT);
    //right store
    defaultPositions[RIGHT_STORE_1] = Position(-118, 110, DEFAULT_HEIGHT);
    defaultPositions[RIGHT_STORE_2] = Position(-122, 158, DEFAULT_HEIGHT);
    defaultPositions[RIGHT_STORE_3] = Position(-126, 205, DEFAULT_HEIGHT);
    defaultPositions[RIGHT_STORE_4] = Position(-124, 254, DEFAULT_HEIGHT);
    defaultPositions[RIGHT_STORE_5] = Position(-121, 300, DEFAULT_HEIGHT);
    //fields
    defaultPositions[CELL_A1] = Position(57, 180, DEFAULT_HEIGHT);
    defaultPositions[CELL_A2] = Position(60, 240, DEFAULT_HEIGHT);
    defaultPositions[CELL_A3] = Position(58, 296, DEFAULT_HEIGHT);
    defaultPositions[CELL_B1] = Position(-0, 181, DEFAULT_HEIGHT);
    defaultPositions[CELL_B2] = Position(-0, 241, DEFAULT_HEIGHT);
    defaultPositions[CELL_B3] = Position(-0, 296, DEFAULT_HEIGHT);
    defaultPositions[CELL_C1] = Position(-53, 179, DEFAULT_HEIGHT);
    defaultPositions[CELL_C2] = Position(-56, 239, DEFAULT_HEIGHT);
    defaultPositions[CELL_C3] = Position(-58, 294, DEFAULT_HEIGHT);
}

int RobotController::move2Pos(robotPosition_t posIndex) {
    return move2Pos(defaultPositions[posIndex]);
}

int RobotController::move2Pos(const Position& pos) {
    cout << "Move to " << pos.getX() << "/" << pos.getY() << "/" << pos.getZ() << " ..." << endl;

    int i = MAX_TRIES;   // Count down to terminate move2Pos after n loops
    do {
            uarmInterface.moveTo(pos.getX(), pos.getY(), pos.getZ(), DEFAULT_SPEED);
            this_thread::sleep_for(chrono::milliseconds(200));
    currentPos = uarmInterface.getPosition();
            cout << "  now at " << currentPos.getX() << "/" << currentPos.getY() << "/" << currentPos.getZ() << endl;		
            cout.flush();
            i--;
    } while (currentPos != pos && i);

    currentPos = pos;
    cout << "... done" << endl;
    return 0;
}

int RobotController::pick() {
    Position p = currentPos;

    move2Pos(p - Position(0, 0, DEFAULT_LOWER));
    uarmInterface.pick(DEFAULT_PICK);
    this_thread::sleep_for(chrono::milliseconds(1000));
    uarmInterface.punp(true);

    return move2Pos(p);
}

int RobotController::place() {
    Position p = currentPos;

    move2Pos(p - Position(0, 0, DEFAULT_LOWER));
    this_thread::sleep_for(chrono::milliseconds(500));
    uarmInterface.punp(false);

    return move2Pos(p);
}

int RobotController::putRobotDown() {
    return move2Pos(currentPos - LIFT_UP);
}

int RobotController::liftRobotUp() {
    return move2Pos(currentPos + LIFT_UP);
}
