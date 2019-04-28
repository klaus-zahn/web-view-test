
/*! @file ipc.c
 * @brief Implements the IPC handling of the template application.
 */
#include <iostream>
using namespace std;

#include "opencv.hpp"


#include "includes.h"
#include "ipc.h"
#include "cgi/cgi.h"


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "RobotController.h"

#ifdef OSC_HOST
#define HTTP_DIR "/var/www/"
//#define HTTP_DIR ""
#else
#define HTTP_DIR "/home/httpd/"
#endif




CIPC::CIPC(CCamera& camera,CImageProcessor& img_process) : m_camera(camera), m_img_process(img_process), m_bInit(false) {
	img_count=0;
}

CIPC::~CIPC() {
        if(m_robot_ctrl) {
            m_robot_ctrl->exit();
        }
}


OSC_ERR CIPC::Init() {
	
	if(m_bInit) return(EALREADY_INITIALIZED);
	
	m_fd=-1;
	
	struct sockaddr_un addr;
	addr.sun_family=AF_UNIX;
	
	OscAssert_w(strlen(CGI_SOCKET_PATH) < sizeof(addr.sun_path), "Path too long");
	
	
	strcpy(addr.sun_path, CGI_SOCKET_PATH);
	unlink(addr.sun_path);
	
	m_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	OscAssert_w(m_socket_fd >= 0, "Cannot open a socket.");
	
	OscAssert_w(fcntl(m_socket_fd, F_SETFL, O_NONBLOCK) == 0
			, "Error setting O_NONBLOCK: %s", strerror(errno));
	
	OscAssert_w(bind(m_socket_fd, (struct sockaddr *) &addr, SUN_LEN(&addr)) == 0
			, "Error binding the socket: %s", strerror(errno));
	
	OscAssert_w(chmod(addr.sun_path, SERV_SOCKET_PERMISSIONS) >= 0
			, "Unable to set access permissions of "
				"socket file node \"%s\"! (%s)", addr.sun_path, strerror(errno));

	OscAssert_w(listen(m_socket_fd, 5) == 0
			, "Error listening on the socket: %s", strerror(errno));
	
	
	/* init the web-settings */
	m_web_settings.exposure_time=INIT_EXPOSURE_TIME;
        m_web_settings.autoExposure = 0;
	m_web_settings.connect = 0;
	
	m_robot_ctrl = RobotController::getInstance();  
	
	m_bInit=true;
	return(SUCCESS);
}

OSC_ERR CIPC::handleIpcRequests() {
	
	if(!m_bInit) return(EGENERAL);
	
	
	struct sockaddr remoteAddr;
	unsigned int remoteAddrLen = sizeof(struct sockaddr);
	
	m_fd = accept(m_socket_fd, &remoteAddr, &remoteAddrLen);
	OSC_ERR err=SUCCESS;
	
	if(m_fd<0) {
		OscAssert_w(errno == EAGAIN, "Error accepting a connection: %s", strerror(errno));
	} else {
		if(fcntl(m_fd, F_SETFL, O_NONBLOCK) != 0) {
			close(m_fd);
			OscMark_format("Error setting O_NONBLOCK: %s", strerror(errno));
			return(EASSERT);
		}
		
		ssize_t numRead, remaining=BUFFER_SIZE;
		char buffer[BUFFER_SIZE+1];
		while((numRead = read(m_fd, buffer+BUFFER_SIZE-remaining, remaining)) != 0) {
			if(numRead > 0) {
				remaining-=numRead;
			}
		}
		buffer[BUFFER_SIZE-remaining]=0;
		m_bHeader_written=false;
		
		if(remaining < BUFFER_SIZE) {
			ProcessRequest(buffer);
		}
		WriteHtmlHeader(HEADER_TEXT_PLAIN); //will be written if not written already
		
		if(m_fd!=-1) close(m_fd);
		m_fd=-1;
	}
	
	return(err);
}

void CIPC::WriteHtmlHeader(HTML_HEADER_TYPE type, int content_length) {
	
	if(m_bHeader_written) return;
	
	switch(type) {
	case HEADER_IMAGE_BMP:
		m_bHeader_written=true;
		
		sprintf(m_buffer,
				"Content-Length: %i\r\n" \
				"Content-Type: image/ x-ms-bmp\r\n" \
				"\r\n"
				, content_length);
		
		IpcWrite(m_buffer, strlen(m_buffer));
		
		break;
        case HEADER_IMAGE_JPG:
		m_bHeader_written=true;
		
		sprintf(m_buffer,
				"Content-Length: %i\r\n" \
				"Content-Type: image/ jpeg\r\n" \
				"\r\n"
				, content_length);
		
		IpcWrite(m_buffer, strlen(m_buffer));
		
		break;
	case HEADER_TEXT_PLAIN:
		m_bHeader_written=true;
		
		const char* b="Status: 200 OK\n" \
				"Content-Type: text/plain\n\n";
		IpcWrite(b, strlen(b));
		
		break;
	}

}

void CIPC::ProcessRequest(char* request) {
	
	char * header;
	
	OscLog(INFO, "IPC Request:%s\n", request);
	
	if(!(header=ReadHeader(&request))) return; /* wrong formated */
	
	//Note: call WriteHtmlHeader BEFORE calling writeArgument
	
	if(strcmp(header, "SetOptions") == 0) {
		while (*request) {
			char * key, * value;
			if(ReadArgument(&request, &key, &value)==SUCCESS) {
			
				if(strcmp(key, "autoExposure") == 0) {
					OscCamSetShutterWidth(0);
				} else if (strcmp(key, "exposureTime") == 0) {
					m_web_settings.exposure_time = strtol(value, NULL, 10)*1000;
					OscCamSetShutterWidth(m_web_settings.exposure_time);
				} else if (strcmp(key, "colorType") == 0) {
					if (strcmp(value, "none") == 0)
						m_camera.setColorType(ColorType_none);
					else if (strcmp(value, "gray") == 0)
						m_camera.setColorType( ColorType_gray);
					else if (strcmp(value, "raw") == 0)
						m_camera.setColorType(ColorType_raw);
					else if (strcmp(value, "debayered") == 0)
						m_camera.setColorType(ColorType_debayered);
				} else if(strcmp(key, "perspective") == 0) {
					m_camera.setPerspective(atoi(value));
				} else if(strcmp(key, "connect") == 0) {
				        if(!m_thread_running) {
				                m_thread_running = true;
				                m_robot_thread = thread(&CIPC::robotThread, this);
				        }
                                        if(m_web_settings.connect == 0) {
                                              m_next_action = CONNECT;
                                              m_web_settings.connect = 1;

                                        } else {
                                              m_next_action = DISCONNECT;
                                              m_web_settings.connect = 0;
                                        }
                                } else if(strcmp(key, "move") == 0) {
                                        if(m_thread_running) {
                                          OscLog(NOTICE, "the following fields are occupied:\n");
                                              for(unsigned int i0 = 0; i0 < m_occupied_fields.size(); i0++) {
                                                  m_game.setField(m_occupied_fields[i0]);
                                                  OscLog(NOTICE, "%s, ", m_occupied_fields[i0].c_str());
                                              }
                                              OscLog(NOTICE, "\n");
                                              m_next_action = MOVE;
                                        } else {
                                              OscLog(ERROR, "robot thread is not running");
                                        }
                                } else if(strcmp(key, "reset") == 0) {
                                        m_game.reset_game();
                                }
			} else {
				*request=0;
			}
		}
                const char * pEnumBuf = NULL;

		WriteHtmlHeader(HEADER_TEXT_PLAIN);
		WriteArgument("colorType", pEnumBuf);
                WriteArgument("perspective", m_camera.getPerspective());
                WriteArgument("autoExposure", m_web_settings.autoExposure);
                WriteArgument("connect", m_robot_ctrl ? m_robot_ctrl->isConnected() : false);
		
	} else if (strcmp(header, "GetImageInfo") == 0) {
		const char * pEnumBuf = NULL;
		
		WriteHtmlHeader(HEADER_TEXT_PLAIN);
		
		WriteArgument("width", m_camera.getROI().width);
		WriteArgument("height", m_camera.getROI().height);
		WriteArgument("exposureTime", (m_web_settings.exposure_time+500)/1000);
		if (m_camera.getColorType() == ColorType_none) {
			pEnumBuf = "none";
		} else if (m_camera.getColorType() == ColorType_gray) {
			pEnumBuf = "gray";
		} else if (m_camera.getColorType() == ColorType_raw) {
			pEnumBuf = "raw";
		} else if (m_camera.getColorType() == ColorType_debayered) {
			pEnumBuf = "debayered";
		}
		WriteArgument("colorType", pEnumBuf);
		WriteArgument("perspective", m_camera.getPerspective());
		WriteArgument("autoExposure", m_web_settings.autoExposure);
		WriteArgument("connect", m_robot_ctrl ? m_robot_ctrl->isConnected() : false);               
		
	} else if (strncmp(header, "GetImage", 8) == 0) {
		
		cv::Mat* img = m_camera.GetLastPicture();
		
		if(img != NULL && !img->empty()) {
			++img_count;
						
			cv::Mat img_write;
			if(m_camera.getPerspective() == 0) {
				/* we show the camera image */
				img_write=*img;
			} else {
				cv::Mat* img_proc=m_img_process.GetProcImage(m_camera.getPerspective()-1);
                                /* in case image is empty -> show camera image*/
                                if(img_proc->empty()) {
                                    img_write=*img;
                                } else {
                                    /* convert to uint8 */
                                    double min_val, max_val;
                                    cv::minMaxLoc(*img_proc, &min_val, &max_val);
                                    img_proc->convertTo(img_write, CV_MAKETYPE(CV_8U,img_proc->depth()), 255.0/(max_val - min_val), -min_val * 255.0/(max_val - min_val));
                                }
			}

			if(WriteImage(img_write) !=SUCCESS) {
				OscLog(ERROR, "Image could not be sent\n");
			}
			
		} else {
			OscLog(ERROR, "Could not Read Latest Picture\n");
		}
	} else if (strcmp(header, "GetSystemInfo") == 0) {
		
		WriteHtmlHeader(HEADER_TEXT_PLAIN);
		
		struct OscSystemInfo * pInfo;
		if(OscCfgGetSystemInfo(&pInfo) == SUCCESS) {
                  /*  WriteArgument("cameraModel", pInfo->hardware.board.revision);
                    WriteArgument("imageSensor", (char*)"Color");
                    WriteArgument("uClinuxVersion", pInfo->software.uClinux.version);*/
		
                    WriteArgument("cameraModel", (char*)"Raspberry Pi Camera");
                    WriteArgument("imageSensor", (char*)"Color");
                    WriteArgument("uClinuxVersion", (char*)"0.9.0");
                }
	}
}


char* CIPC::strtrim(char * str) {
	char * end = strchr(str, 0) - 1;

	while (*str != 0 && strchr(" \t\n", *str) != NULL)
		str += 1;

	while (end > str && strchr(" \t\n", *end) != NULL)
		end -= 1;

	*(end + 1) = 0;

	return str;
}

char* CIPC::ReadHeader(char ** request) {
	char* newline = strchr(*request, '\n');
	if(!newline) return(NULL);
	
	*newline = 0;
	char* ret = strtrim(*request);
	*request = newline + 1;
	return(ret);
}

OSC_ERR CIPC::ReadArgument(char ** pBuffer, char ** pKey, char ** pValue) {
	char * newline, * colon;
	
	if(!(newline = strchr(*pBuffer, '\n'))) return(EINVALID_PARAMETER);
	*newline = 0;
	
	if(!(colon = strchr(*pBuffer, ':'))) return(EINVALID_PARAMETER);
	*colon = 0;
		
	*pKey = strtrim(*pBuffer);
	*pValue = strtrim(colon + 1);
	*pBuffer = newline + 1;
	return(SUCCESS);
}

OSC_ERR CIPC::WriteArgument(const char * pKey, int value) {
	int n = snprintf(m_buffer, m_buffer_count, "%s: %i\n", pKey, value);
	
	if( IpcWrite(m_buffer, (size_t)n) < 0) return(EFILE_ERROR);
	return(SUCCESS);
}

OSC_ERR CIPC::WriteArgument(const char * pKey, const char * pValue) {
	int n = snprintf(m_buffer, m_buffer_count, "%s: %s\n", pKey, pValue);
	
	if( IpcWrite(m_buffer, (size_t)n) < 0) return(EFILE_ERROR);
	return(SUCCESS);
}

OSC_ERR CIPC::WriteImage(const cv::Mat img) {
	
        std::vector<int> qualityType;
        qualityType.push_back(CV_IMWRITE_JPEG_QUALITY);
        qualityType.push_back(90);

        std::vector<uchar> buf;
        cv::imencode(".jpg", img, buf, qualityType);
    
	WriteHtmlHeader(HEADER_IMAGE_JPG, buf.size());
	
	//write image data
	if(IpcWrite(buf.data(), (size_t)buf.size()) <=0)
			return(EGENERAL);
	
	return(SUCCESS);
}

int CIPC::IpcWrite(const void* buf, size_t count) {
	if(m_fd < 0) return(m_fd);
	int ret;
	while((ret=write(m_fd, buf, count)) == -1 
			&& (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)) {
		//resource temp. unavailable -> just try again
		usleep(1000);
	}
	if(ret==-1) {
		close(m_fd);
		m_fd=-1;
	}
	return(ret);
}

void CIPC::robotThread(void)
{
  while(m_thread_running) {

    if(m_next_action == CONNECT) {
        m_next_action = NO_ACTION;
        if (m_robot_ctrl->init(PORT)) {
            OscLog(ERROR, "connecting to '%s' failed!\n", PORT);
        } else {
            OscLog(NOTICE, "connect to '%s' successful\n", PORT);
        }
    } else if(m_next_action == MOVE) {
        m_next_action = NO_ACTION;
        string next_field = m_game.getNextField();
        int num_move = m_game.getMove();

        OscLog(NOTICE, "move %d, to field %s\n", num_move, next_field.c_str());

        if(next_field.size() > 0 && num_move <= 5) {

            m_robot_ctrl->move2Pos((robotPosition_t) (6+num_move));//we start with 1 -> RIGHT_STORE_1 = 7
            m_robot_ctrl->pick();

            if(next_field == "A1") {
              m_robot_ctrl->move2Pos(CELL_A1);
            } else if(next_field == "A2") {
              m_robot_ctrl->move2Pos(CELL_A2);
            } else if(next_field == "A3") {
              m_robot_ctrl->move2Pos(CELL_A3);
            } else if(next_field == "B1") {
              m_robot_ctrl->move2Pos(CELL_B1);
            } else if(next_field == "B2") {
              m_robot_ctrl->move2Pos(CELL_B2);
            } else if(next_field == "B3") {
              m_robot_ctrl->move2Pos(CELL_B3);
            } else if(next_field == "C1") {
              m_robot_ctrl->move2Pos(CELL_C1);
            } else if(next_field == "C2") {
              m_robot_ctrl->move2Pos(CELL_C2);
            } else if(next_field == "C3") {
              m_robot_ctrl->move2Pos(CELL_C3);
            }
            m_robot_ctrl->place();

            m_robot_ctrl->move2Pos(PARKING_1);
        }

    } else if(m_next_action == DISCONNECT) {
        m_next_action = NO_ACTION;
        m_robot_ctrl->exit();
        OscLog(NOTICE, "disconnected from '%s' successful\n", PORT);
        m_web_settings.connect = 0;
    }

    this_thread::sleep_for(chrono::milliseconds(100));
  }
}







