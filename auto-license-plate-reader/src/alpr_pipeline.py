import cv2
import easyocr
from picamera2 import Picamera2
from ultralytics import YOLO

class Pipeline:
    def __init__(self, model):
        try:
            self.model = YOLO(model)
        except:
            raise ValueError("Invalid model supplied!")

        # initialize camera
        try:
            self.picam2 = Picamera2()
            self.picam2.preview_configuration.main.size = (1080, 720)
            self.picam2.preview_configuration.main.format = "RGB888"
            self.picam2.configure("preview")
            self.picam2.start()
        except:
            raise ValueError("Camera failed to initialize!")

        try:
            self.reader = easyocr.Reader(['en'], gpu=False)
        except:
            raise ValueError("Reader failed to initialize!")

    def take_picture(self):
        # capture frame
      frame = self.picam2.capture_array()
      results = self.model(frame, imgsz = 320, save_crop=False)

      # process images
      for license_plate in results[0].boxes.data.tolist():
          x1, y1, x2, y2, score, class_id = license_plate

          # crop license plate
          license_plate_crop = frame[int(y1):int(y2), int(x1):int(x2), :]


          # processed license plate
          license_plate_crop_gray = cv2.cvtColor(license_plate_crop, cv2.COLOR_BGR2GRAY)
          blurred = cv2.GaussianBlur(license_plate_crop_gray, (5,5),0)
          _, license_plate_crop_thresh = cv2.threshold(blurred, 64, 255, cv2.THRESH_BINARY_INV)

          read_result = self.reader.readtext(
          license_plate_crop_thresh,
          allowlist='ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789');

          for (bbox, read_text, confidence) in read_result:
                return read_text

      # No plate grabbed
      return "";

#cv2.destroyAllWindows()
#picam2.stop()