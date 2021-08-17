import cv2
import time

cap = cv2.VideoCapture("/home/yuan/yanxi-1080p-2M.mp4")
fcc = cv2.VideoWriter_fourcc('a', 'v', 'c', '1')
out = cv2.VideoWriter()
out.open(filename="test2.mp4", fourcc=fcc, fps=25, frameSize=(1920,1080), encodeParams="bitrate=1000")

while cap.isOpened():
    ret, img0 = cap.read()
    if not ret:
        print("Finished to read the video!")
        break;
    starttime = time.time()
    out.write(img0)
    endtime = time.time()
    print((endtime-starttime))

cap.release()
out.release()
