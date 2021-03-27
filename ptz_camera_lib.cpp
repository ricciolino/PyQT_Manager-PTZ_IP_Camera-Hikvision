#include <iostream>
#include <string>
#include <sstream>
#include <iostream>
#include <cstring> // memcpy
#include <mutex>
#include <iomanip>
#include <opencv2/highgui.hpp>
#include "./hikvisionSDK-V6.1.6.3_build20200925/include/HCNetSDK.h"

// compile: g++ -shared -Wl,-soname,testlib -o ptz_camera_lib.so -fPIC ptz_camera_lib.cpp -I/usr/local/include/opencv4 -L/usr/local/lib -I"./hikvisionSDK-V6.1.6.3_build20200925/include/" -L"./hikvisionSDK-V6.1.6.3_build20200925/lib/" -Wl,-rpath,'hikvisionSDK-V6.1.6.3_build20200925/lib' `pkg-config --libs opencv4` -lpthread -lAudioRender -lHCCore -lSuperRender -lhcnetsdk -lhpr -lssl

extern "C" {
    bool ping(const char* address_cstr) {
        std::string address(address_cstr);
        std::string command = "ping -c 2 " + address + " 2>&1";
        FILE *in;
        char buff[512];
        const char mode[] = {"r"};
        if(!(in = popen(command.c_str(), mode))) { return 1; }
        while(fgets(buff, sizeof(buff), in)!=NULL) { }
        if( pclose(in) != 0 ) {
            std::cout << "[ptz_camera_lib]: Cannot PING the camera..." << std::endl;
            NET_DVR_Cleanup();
            return false;
        }
        return true;
    }
    bool init_and_connect(const char* address_cstr, int port, const char* user_cstr, const char* psw_cstr) {
        std::string address(address_cstr);
        std::string user(user_cstr);
        std::string psw(psw_cstr);
        std::cout << "[ptz_camera_lib]: Camera initialization..." << std::endl;
        NET_DVR_USER_LOGIN_INFO struLoginInfo = {0};
        NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = {0};
        NET_DVR_PTZPOS struPtzPos = {0};
        int m_idUserConnected = -1;
        if (!NET_DVR_Init()) {
            std::cout << "[ptz_camera_lib]: Init error, %d\n" << NET_DVR_GetLastError() << std::endl;
            NET_DVR_Cleanup();
            return false;
        }
        std::cout << "[ptz_camera_lib]: Connecting to camera..." << std::endl;
        std::string command = "ping -c 2 " + address + " 2>&1";
        FILE *in;
        char buff[512];
        const char mode[] = {"r"};
        if(!(in = popen(command.c_str(), mode))) { return 1; }
        while(fgets(buff, sizeof(buff), in)!=NULL) { }
        if( pclose(in) != 0 ) {
            std::cout << "[ptz_camera_lib]: Cannot PING the camera..." << std::endl;
            NET_DVR_Cleanup();
            return false;
        }
        struLoginInfo.wPort = port;
        memcpy(struLoginInfo.sDeviceAddress, address.c_str(), NET_DVR_DEV_ADDRESS_MAX_LEN);
        memcpy(struLoginInfo.sUserName, user.c_str(), NAME_LEN);
        memcpy(struLoginInfo.sPassword, psw.c_str(), PASSWD_LEN);
        m_idUserConnected = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);
        if (m_idUserConnected < 0) {
            std::cout << "[ptz_camera_lib]: Login error, %d\n" << NET_DVR_GetLastError() << std::endl;
            NET_DVR_Cleanup();
            return false;
        }
        std::cout << "[ptz_camera_lib]: Connection succesful" << std::endl;
        return true;
    }
    bool set_pos(const float x, const float y, const float z) {
        std::cout << "[ptz_camera_lib]: Setting position..." << std::endl;
        const int wPanPosDec = (int)(x*10);
        WORD wPanPosHex = (wPanPosDec / 1000) * 4096 + ((wPanPosDec % 1000) / 100) * 256 + ((wPanPosDec % 100) / 10) * 16 + (wPanPosDec % 10);
        const int wTiltPosDec = (int)(y*10);
        WORD wTiltPosHex = (wTiltPosDec / 1000) * 4096 + ((wTiltPosDec % 1000) / 100) * 256 + ((wTiltPosDec % 100) / 10) * 16 + (wTiltPosDec % 10);
        const int wZoomPosDec = (int)(z*10);
        WORD wZoomPosHex = (wZoomPosDec / 1000) * 4096 + ((wZoomPosDec % 1000) / 100) * 256 + ((wZoomPosDec % 100) / 10) * 16 + (wZoomPosDec % 10);
        NET_DVR_PTZPOS struSetPtzPos;
        struSetPtzPos.wPanPos = wPanPosHex;
        struSetPtzPos.wTiltPos = wTiltPosHex;
        struSetPtzPos.wAction = 1;
        struSetPtzPos.wZoomPos = wZoomPosHex;
        if (!NET_DVR_SetDVRConfig(0, NET_DVR_SET_PTZPOS, 1, &struSetPtzPos, sizeof (NET_DVR_PTZPOS))) {
            std::cout << "[ptz_camera_lib]: Error setting position: " << NET_DVR_GetLastError() << std::endl;
            return false;
        }
        return true;
    }
    bool move(const int speed, const char* dir_cstr) {
        std::string dir(dir_cstr);
        if (dir == "left") {
            if( !NET_DVR_PTZControlWithSpeed_Other(0, 1, PAN_LEFT, 0, speed)){
                std::cout << "[ptz_camera_lib]: Can't move left..." << std::endl;
                return false;
            }
        } else if (dir == "up") {
            if( !NET_DVR_PTZControlWithSpeed_Other(0, 1, TILT_UP, 0, speed)){
                std::cout << "[ptz_camera_lib]: Can't move up..." << std::endl;
                return false;
            }
        } else if (dir == "right") {
            if( !NET_DVR_PTZControlWithSpeed_Other(0, 1, PAN_RIGHT, 0, speed)){
                std::cout << "[ptz_camera_lib]: Can't move right..." << std::endl;
                return false;
            }
        } else if (dir == "down") {
            if( !NET_DVR_PTZControlWithSpeed_Other(0, 1, TILT_DOWN, 0, speed)){
                std::cout << "[ptz_camera_lib]: Can't move down..." << std::endl;
                return false;
            }
        } else if (dir == "zoomi") {
            if( !NET_DVR_PTZControl_Other(0, 1, ZOOM_IN, 0)){
                std::cout << "[ptz_camera_lib]: Can't zoom in..." << std::endl;
                return false;
            }
        } else if (dir == "zoomo") {
            if( !NET_DVR_PTZControl_Other(0, 1, ZOOM_OUT, 0)){
                std::cout << "[ptz_camera_lib]: Can't zoom out..." << std::endl;
                return false;
            }
        } else if (dir == "stop") {
            if( !NET_DVR_PTZControlWithSpeed_Other(0, 1, PAN_LEFT, 1, 1)){
                std::cout << "[ptz_camera_lib]: Can't stop the movement..." << std::endl;
                return false;
            }
        } else {
            std::cout << "[ptz_camera_lib]: No movement specified..." << std::endl;
            return false;
        }
        return true;
    }
    bool move_step(const int step, const char* dir_cstr) {
        DWORD dwReturn = 0;
        NET_DVR_PTZPOS struPtzPos = {0};
        if( !NET_DVR_GetDVRConfig(0, NET_DVR_GET_PTZPOS, 1, &struPtzPos, sizeof (struPtzPos), &dwReturn)) {
            std::cout << "[ptz_camera_lib]: Cannot get PTZ..." << std::endl;
            return false;
        }
        WORD wPanPosHex = (struPtzPos.wPanPos / 4096) * 1000 + ((struPtzPos.wPanPos % 4096) / 256) * 100 + ((struPtzPos.wPanPos % 256) / 16) * 10 + (struPtzPos.wPanPos % 16);
        float m_nPtzXPos = (float) wPanPosHex / 10;
        WORD wTiltPosHex = (struPtzPos.wTiltPos / 4096) * 1000 + ((struPtzPos.wTiltPos % 4096) / 256) * 100 + ((struPtzPos.wTiltPos % 256) / 16) * 10 + (struPtzPos.wTiltPos % 16);
        float m_nPtzYPos = (float) wTiltPosHex / 10;
        WORD wZoomPosHex = (struPtzPos.wZoomPos / 4096) * 1000 + ((struPtzPos.wZoomPos % 4096) / 256) * 100 + ((struPtzPos.wZoomPos % 256) / 16) * 10 + (struPtzPos.wZoomPos % 16);
        float m_nPtzZPos = (float) wZoomPosHex / 10;
        std::cout << "[ptz_camera_lib]: Setting position..." << std::endl;
        std::string dir(dir_cstr);
        if (dir == "left") {
            m_nPtzXPos -= step;
            if(m_nPtzXPos < 0) {
                m_nPtzXPos = 360 + m_nPtzXPos;
            }
        } else if (dir == "up") {
            m_nPtzYPos -= step;
            if(m_nPtzYPos < 0) {
                m_nPtzYPos = 0;
            }
        } else if (dir == "right") {
            m_nPtzXPos += step;
            if(m_nPtzXPos > 360) {
                m_nPtzXPos -= 360;
            }
        } else if (dir == "down") {
            m_nPtzYPos += step;
            if(m_nPtzYPos > 90) {
                m_nPtzYPos = 90;
            }
        } else if (dir == "zoomi") {
            m_nPtzZPos += step;
            if(m_nPtzZPos > 25) {
                m_nPtzYPos = 25;
            }
        } else if (dir == "zoomo") {
            m_nPtzZPos -= step;
            if(m_nPtzZPos < 1) {
                m_nPtzZPos = 1;
            }
        } else {
            std::cout << "[ptz_camera_lib]: No position specified..." << std::endl;
            return false;
        }
        const int wPanPosDec = (int)(m_nPtzXPos*10);
        wPanPosHex = (wPanPosDec / 1000) * 4096 + ((wPanPosDec % 1000) / 100) * 256 + ((wPanPosDec % 100) / 10) * 16 + (wPanPosDec % 10);
        const int wTiltPosDec = (int)(m_nPtzYPos*10);
        wTiltPosHex = (wTiltPosDec / 1000) * 4096 + ((wTiltPosDec % 1000) / 100) * 256 + ((wTiltPosDec % 100) / 10) * 16 + (wTiltPosDec % 10);
        const int wZoomPosDec = (int)(m_nPtzZPos*10);
        wZoomPosHex = (wZoomPosDec / 1000) * 4096 + ((wZoomPosDec % 1000) / 100) * 256 + ((wZoomPosDec % 100) / 10) * 16 + (wZoomPosDec % 10);
        NET_DVR_PTZPOS struSetPtzPos;
        struSetPtzPos.wPanPos = wPanPosHex;
        struSetPtzPos.wTiltPos = wTiltPosHex;
        struSetPtzPos.wAction = 1;
        struSetPtzPos.wZoomPos = wZoomPosHex;
        if (!NET_DVR_SetDVRConfig(0, NET_DVR_SET_PTZPOS, 1, &struSetPtzPos, sizeof (NET_DVR_PTZPOS))) {
            std::cout << "[ptz_camera_lib]: Error setting position: " << NET_DVR_GetLastError() << std::endl;
            return false;
        }
        return true;
    }
    bool print_pos() {
        NET_DVR_PTZPOS struPtzPos = {0};
        DWORD dwReturn = 0;
        if( !NET_DVR_GetDVRConfig(0, NET_DVR_GET_PTZPOS, 1, &struPtzPos, sizeof (struPtzPos), &dwReturn)) {
            std::cout << "[ptz_camera_lib]: Cannot get PTZ..." << std::endl;
            return false;
        } else {
            WORD wPanPosHex = (struPtzPos.wPanPos / 4096) * 1000 + ((struPtzPos.wPanPos % 4096) / 256) * 100 + ((struPtzPos.wPanPos % 256) / 16) * 10 + (struPtzPos.wPanPos % 16);
            float m_nPtzXPos = (float) wPanPosHex / 10;
            WORD wTiltPosHex = (struPtzPos.wTiltPos / 4096) * 1000 + ((struPtzPos.wTiltPos % 4096) / 256) * 100 + ((struPtzPos.wTiltPos % 256) / 16) * 10 + (struPtzPos.wTiltPos % 16);
            float m_nPtzYPos = (float) wTiltPosHex / 10;
            WORD wZoomPosHex = (struPtzPos.wZoomPos / 4096) * 1000 + ((struPtzPos.wZoomPos % 4096) / 256) * 100 + ((struPtzPos.wZoomPos % 256) / 16) * 10 + (struPtzPos.wZoomPos % 16);
            float m_nPtzZPos = (float) wZoomPosHex / 10;
            std::stringstream stream;
            stream << std::fixed << std::setprecision(1) << m_nPtzXPos;
            std::string m_nPtzXPosStr = stream.str();
            stream.str("");
            stream.clear();
            stream << std::fixed << std::setprecision(1) << m_nPtzYPos;
            std::string m_nPtzYPosStr = stream.str();
            stream.str("");
            stream.clear();
            stream << std::fixed << std::setprecision(1) << m_nPtzZPos;
            std::string m_nPtzZPosStr = stream.str();
            std::cout << "PTZ: " << m_nPtzXPosStr << ", " << m_nPtzYPosStr << ", " << m_nPtzZPosStr << std::endl;
        }
        return true;
    }
    float get_Xpos() {
        NET_DVR_PTZPOS struPtzPos = {0};
        DWORD dwReturn = 0;
        if( !NET_DVR_GetDVRConfig(0, NET_DVR_GET_PTZPOS, 1, &struPtzPos, sizeof (struPtzPos), &dwReturn)) {
            std::cout << "[ptz_camera_lib]: Cannot get X PTZ position..." << std::endl;
            return -1;
        } else {
            WORD wPanPosHex = (struPtzPos.wPanPos / 4096) * 1000 + ((struPtzPos.wPanPos % 4096) / 256) * 100 + ((struPtzPos.wPanPos % 256) / 16) * 10 + (struPtzPos.wPanPos % 16);
            float m_nPtzXPos = (float) wPanPosHex / 10;
            return m_nPtzXPos;
        }
    }
    float get_Ypos() {
        NET_DVR_PTZPOS struPtzPos = {0};
        DWORD dwReturn = 0;
        if( !NET_DVR_GetDVRConfig(0, NET_DVR_GET_PTZPOS, 1, &struPtzPos, sizeof (struPtzPos), &dwReturn)) {
            std::cout << "[ptz_camera_lib]: Cannot get Y PTZ position..." << std::endl;
            return -1;
        } else {
            WORD wTiltPosHex = (struPtzPos.wTiltPos / 4096) * 1000 + ((struPtzPos.wTiltPos % 4096) / 256) * 100 + ((struPtzPos.wTiltPos % 256) / 16) * 10 + (struPtzPos.wTiltPos % 16);
            float m_nPtzYPos = (float) wTiltPosHex / 10;
            return m_nPtzYPos;
        }
    }
    float get_Zpos() {
        NET_DVR_PTZPOS struPtzPos = {0};
        DWORD dwReturn = 0;
        if( !NET_DVR_GetDVRConfig(0, NET_DVR_GET_PTZPOS, 1, &struPtzPos, sizeof (struPtzPos), &dwReturn)) {
            std::cout << "[ptz_camera_lib]: Cannot get Z PTZ position..." << std::endl;
            return -1;
        } else {
            WORD wZoomPosHex = (struPtzPos.wZoomPos / 4096) * 1000 + ((struPtzPos.wZoomPos % 4096) / 256) * 100 + ((struPtzPos.wZoomPos % 256) / 16) * 10 + (struPtzPos.wZoomPos % 16);
            float m_nPtzZPos = (float) wZoomPosHex / 10;
            return m_nPtzZPos;
        }
    }
    bool disconnect() {
        if( !NET_DVR_Logout_V30(0)){
            std::cout << "[ptz_camera_lib]: Error in NET_DVR_Logout_V30" << std::endl;
        }
        std::cout << "[ptz_camera_lib]: Logout succesful..." << std::endl;
        if ( !NET_DVR_Cleanup()){
            std::cout << "[ptz_camera_lib]: Error in NET_DVR_Cleanup" << std::endl;
        }
        std::cout << "[ptz_camera_lib]: Cleanup succesful..." << std::endl;
        std::cout << "[ptz_camera_lib]: Disconnected" << std::endl;
        return true;
    }
}
