import cv2qq
import numpy as np
import urllib.request
import time
from datetime import datetime
from pathlib import Path

# ==========================================
# ESP32-CAM SETTINGS
# ==========================================

cameraIP = "10.71.243.216"

url = f"http://{cameraIP}/cam-hi.jpg"

# Sends a signal to ESP32-CAM to flash the LED
ledURL = f"http://{cameraIP}/person-detected"

# ==========================================
# YOLO SETTINGS
# ==========================================

whT = 320
confThreshold = 0.5
nmsThreshold = 0.3

# ==========================================
# IMAGE CAPTURE SETTINGS
# ==========================================

captureCooldown = 15
lastCaptureTime = 0

captureFolder = Path(
    r"C:\Users\Rasyad Anuar\Pictures\Saved Pictures"
)

captureFolder.mkdir(parents=True, exist_ok=True)

# ==========================================
# YOLO FILES
# ==========================================

folder = Path(__file__).resolve().parent

classesfile = folder / "coco.names"
modelConfig = folder / "yolov3.cfg"
modelWeights = folder / "yolov3.weights"

# Load object class names
with open(classesfile, "r") as f:
    classNames = f.read().splitlines()

# Load YOLOv3 model
net = cv2.dnn.readNetFromDarknet(
    str(modelConfig),
    str(modelWeights)
)

net.setPreferableBackend(cv2.dnn.DNN_BACKEND_OPENCV)
net.setPreferableTarget(cv2.dnn.DNN_TARGET_CPU)

outputNames = net.getUnconnectedOutLayersNames()


# ==========================================
# LED SIGNAL FUNCTION
# ==========================================

def flashLED():
    try:
        request = urllib.request.Request(
            ledURL,
            data=b"",
            method="POST"
        )

        with urllib.request.urlopen(
            request, timeout=2
        ) as response:
            response.read()

        print("LED flash signal sent!")

    except Exception as e:
        print("Failed to trigger LED:", e)


# ==========================================
# OBJECT DETECTION FUNCTION
# ==========================================

def findObject(outputs, im):

    hT, wT = im.shape[:2]

    bbox = []
    classIds = []
    confs = []

    personDetected = False

    for output in outputs:
        for det in output:

            scores = det[5:]
            classId = int(np.argmax(scores))
            confidence = float(scores[classId])

            if confidence > confThreshold:

                w = int(det[2] * wT)
                h = int(det[3] * hT)

                x = int(det[0] * wT - w / 2)
                y = int(det[1] * hT - h / 2)

                bbox.append([x, y, w, h])
                classIds.append(classId)
                confs.append(confidence)

    # Remove overlapping bounding boxes
    indices = cv2.dnn.NMSBoxes(
        bbox,
        confs,
        confThreshold,
        nmsThreshold
    )

    for i in np.array(indices).flatten():

        i = int(i)

        x, y, w, h = bbox[i]
        label = classNames[classIds[i]]

        # ==================================
        # PERSON DETECTION
        # ==================================

        if label == "person":

            personDetected = True

            # Green bounding box for person
            color = (0, 255, 0)

        else:

            # Purple bounding box for other objects
            color = (255, 0, 255)

        # Draw bounding box for ALL objects
        cv2.rectangle(
            im,
            (x, y),
            (x + w, y + h),
            color,
            2
        )

        # Display name and confidence
        text = (
            f"{label.upper()} "
            f"{int(confs[i] * 100)}%"
        )

        cv2.putText(
            im,
            text,
            (x, max(y - 10, 20)),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.6,
            color,
            2
        )

    return personDetected


# ==========================================
# MAIN PROGRAM
# ==========================================

print("ESP32-CAM Object Detection Started")
print("Detecting all COCO objects")
print("Saving images only when person detected")
print("Capture interval: 15 seconds")
print("Save location:", captureFolder)
print("Press Q to quit")

try:

    while True:

        # Get ESP32-CAM image
        with urllib.request.urlopen(
            url, timeout=5
        ) as response:
            img_bytes = response.read()

        imgnp = np.frombuffer(
            img_bytes,
            dtype=np.uint8
        )

        im = cv2.imdecode(
            imgnp,
            cv2.IMREAD_COLOR
        )

        if im is None:
            print("Failed to decode image")
            continue

        # Create YOLO input
        blob = cv2.dnn.blobFromImage(
            im,
            1 / 255,
            (whT, whT),
            (0, 0, 0),
            swapRB=True,
            crop=False
        )

        net.setInput(blob)

        # Run YOLOv3
        outputs = net.forward(outputNames)

        # Detect all objects
        personDetected = findObject(
            outputs,
            im
        )

        currentTime = time.monotonic()

        # ==================================
        # SAVE ONLY WHEN PERSON DETECTED
        # ==================================

        if personDetected:

            cv2.putText(
                im,
                "PERSON DETECTED",
                (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.8,
                (0, 0, 255),
                2
            )

            if (
                currentTime - lastCaptureTime
                >= captureCooldown
            ):

                timestamp = datetime.now().strftime(
                    "%Y%m%d_%H%M%S_%f"
                )

                filename = (
                    captureFolder
                    / f"person_{timestamp}.jpg"
                )

                # Save image
                saved = cv2.imwrite(
                    str(filename),
                    im
                )

                if saved:

                    print("PERSON DETECTED!")
                    print("Image saved:", filename)

                    # Trigger ESP32-CAM flash LED
                    flashLED()

                    # Start cooldown
                    lastCaptureTime = time.monotonic()

                else:
                    print("Failed to save image")

        # ==================================
        # DISPLAY CAMERA
        # ==================================

        cv2.imshow(
            "ESP32-CAM Object Detection",
            im
        )

        # Press Q to stop
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

except (OSError, urllib.error.URLError) as e:
    print("Camera connection error:", e)

except KeyboardInterrupt:
    print("Program stopped by user")

finally:
    cv2.destroyAllWindows()
    print("Camera stopped")
