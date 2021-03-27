'''
Simple PyQt-window application to handle the movement of an IP camera by Hikvision.
Author: Nicholas Redi
email: nicholasredimail@gmail.com
'''
import sys
import ctypes
import numpy as np
import cv2
from PyQt5.QtGui import QPixmap,QImage
from PyQt5.QtWidgets import QWidget, QApplication, QLabel, QVBoxLayout
from PyQt5.QtCore import pyqtSignal, pyqtSlot, Qt, QThread

# dynamic library
camera = ctypes.cdll.LoadLibrary('./ptz_camera_lib.so')

# connection params
address = 'xxx.xxx.xxx.xxx'
user = 'xxxxxx'
psw = 'xxxxxx'
ctype_address = ctypes.create_string_buffer(b'xxx.xxx.xxx.xxx')
ctype_user = ctypes.create_string_buffer(b'xxxxxx')
ctype_psw = ctypes.create_string_buffer(b'xxxxxx')
port = 8000

# ptz params
pt_speed = 4 # range 1 - 7
left = ctypes.create_string_buffer(b'left')
down = ctypes.create_string_buffer(b'down')
up = ctypes.create_string_buffer(b'up')
right = ctypes.create_string_buffer(b'right')
stop = ctypes.create_string_buffer(b'stop')
zoomi = ctypes.create_string_buffer(b'zoomi')
zoomo = ctypes.create_string_buffer(b'zoomo')

class VideoThread(QThread):
    """
    Thread class to acquire frames.
    """
    change_pixmap_signal = pyqtSignal(np.ndarray)

    def __init__(self):
        """
        Initialize stream and flag to stop acquisition.
        """
        super().__init__()
        self.run_flag = True
        self.video_capture = cv2.VideoCapture(f"rtsp://{user}:{psw}@{address}")

    def run(self):
        """
        Acquire frames until the stream is open and no stop signal is recieved.
        """
        while self.video_capture.isOpened() and self.run_flag:
            ret, frame = self.video_capture.read()
            if not ret:
                print("Cannot grab the frame. Camera reinitialization...")
                self.video_capture = cv2.VideoCapture(f"rtsp://{user}:{psw}@{address}")
                continue
            self.change_pixmap_signal.emit(frame)
        self.video_capture.release()

    def stop(self):
        """
        Sets run_flag to False and waits for thread to finish.
        """
        self.run_flag = False
        self.wait()

class PTZManager(QWidget):
    """
    The window with a label to show the stream.
    """
    def __init__(self):
        """
        Initialize the window layout.
        Initialize and connect to the camera.
        Starts the VideoThread.
        """
        super().__init__()

        # window layout
        self.setWindowTitle("PTZ Manager")
        self.image_label = QLabel()
        vbox = QVBoxLayout()
        vbox.setContentsMargins(0, 0, 0, 0)
        vbox.addWidget(self.image_label)
        self.setLayout(vbox)

        # camera connection
        if not camera.init_and_connect(ctype_address, port, ctype_user, ctype_psw):
            sys.exit()

        # thread definition
        self.thread = VideoThread()
        self.thread.change_pixmap_signal.connect(self.update_image)
        self.thread.start()

    def closeEvent(self, event):
        """
        Close event for the window.
        """
        if self.thread is not None:
            self.thread.stop()
        camera.disconnect()
        event.accept()

    def keyPressEvent(self, event):
        """
        Key press event for the window.
        """
        # if a key is holding down do not handle the event
        # the camera will continue to move untile the key is released
        if event.isAutoRepeat():
            return

        # press V to print the position to the console
        if event.key() == Qt.Key_V:
            camera.print_pos()
            return

        # handle to key pressed to move the camera
        # press Q to exit the program
        if event.key() == Qt.Key_Right:
            camera.move(pt_speed, right)
        elif event.key() == Qt.Key_Left:
            camera.move(pt_speed, left)
        elif event.key() == Qt.Key_Up:
            camera.move(pt_speed, up)
        elif event.key() == Qt.Key_Down:
            camera.move(pt_speed, down)
        elif event.key() == Qt.Key_Plus:
            camera.move(0, zoomi)
        elif event.key() == Qt.Key_Minus:
            camera.move(0, zoomo)
        elif event.key() == Qt.Key_Q:
            self.close()

    def keyReleaseEvent(self, event):
        """
        Key release event for the window.
        """
        # stop the camera movement when the key is released
        if (event.key() == Qt.Key_Right or\
                event.key() == Qt.Key_Left or\
                event.key() == Qt.Key_Up or\
                event.key() == Qt.Key_Down or\
                event.key() == Qt.Key_Plus or\
                event.key() == Qt.Key_Minus) and\
                not event.isAutoRepeat():
            camera.move(1, stop)

    @pyqtSlot(np.ndarray)
    def update_image(self, cv_img):
        """
        Updates the image_label with the openCV image from the stream captured by the thread.
        """
        self.image_label.setPixmap(convert_cv_qt(cv_img))

def convert_cv_qt(cv_img):
    """
    Convert an openCV image to QPixmap.
    """
    rgb_image = cv2.cvtColor(cv_img, cv2.COLOR_BGR2RGB)
    h, w, ch = rgb_image.shape
    bytes_per_line = ch * w
    qt_image = QImage(rgb_image.data, w, h, bytes_per_line, QImage.Format_RGB888)
    return QPixmap.fromImage(qt_image.scaled(w, h, Qt.KeepAspectRatio))

if __name__ == "__main__":
    app = QApplication(["PTZ Manager"])
    a = PTZManager()
    a.show()
    sys.exit(app.exec_())
