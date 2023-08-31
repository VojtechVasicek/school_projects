import numpy as np
import imutils
import cv2
import os
import time
import sys
import metrics

# Check if all arguments are valid
if len(sys.argv) != 4:
	print("This script takes 3 arguments.")
	exit()
img_dir = metrics.check_arg(sys.argv[1], "images")
label_dir = metrics.check_arg(sys.argv[2], "labels")
dst_dir = metrics.check_arg(sys.argv[3], "result")

dir_list = os.listdir(img_dir)
ious = []
# Cycle through all images in given directory
cycle_times = []
for img_name in dir_list:
	# Check if provided file is image, if not, the file gets skipped
	if(metrics.is_image(img_dir + img_name)):
		start_time = time.time()
		img = cv2.imread(img_dir + img_name)
		file_path = metrics.change_file_format(label_dir + img_name, "txt")
		with open(file_path, 'r') as file:
			detected_boxes = []
			# Load ground truth bounding boxes as gt_boxes
			gt_boxes = metrics.get_boxes(file_path)
			image = cv2.imread(img_dir + img_name)

			# This starts the part that was taken from https://pyimagesearch.com/2014/11/24/detecting-barcodes-images-python-opencv/
			#################################################################
			gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

			# compute the Scharr gradient magnitude representation of the images
			# in both the x and y direction using OpenCV 2.4
			ddepth = cv2.cv.CV_32F if imutils.is_cv2() else cv2.CV_32F
			gradX = cv2.Sobel(gray, ddepth=ddepth, dx=1, dy=0, ksize=-1)
			gradY = cv2.Sobel(gray, ddepth=ddepth, dx=0, dy=1, ksize=-1)

			# subtract the y-gradient from the x-gradient
			gradient = cv2.subtract(gradX, gradY)
			gradient = cv2.convertScaleAbs(gradient)

			# blur and threshold the image
			blurred = cv2.blur(gradient, (9, 9))
			(_, thresh) = cv2.threshold(blurred, 225, 255, cv2.THRESH_BINARY)

			# construct a closing kernel and apply it to the thresholded image
			kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (21, 7))
			closed = cv2.morphologyEx(thresh, cv2.MORPH_CLOSE, kernel)

			# perform a series of erosions and dilations
			closed = cv2.erode(closed, None, iterations = 4)
			closed = cv2.dilate(closed, None, iterations = 4)

			# find the contours in the thresholded image, then sort the contours
			# by their area, keeping only the largest one
			cnts = cv2.findContours(closed.copy(), cv2.RETR_EXTERNAL,
				cv2.CHAIN_APPROX_SIMPLE)
			cnts = imutils.grab_contours(cnts)
			if(len(sorted(cnts, key = cv2.contourArea, reverse = True)) >= 1):
				c = sorted(cnts, key = cv2.contourArea, reverse = True)[0]
				rect = cv2.minAreaRect(c)
				box = cv2.cv.BoxPoints(rect) if imutils.is_cv2() else cv2.boxPoints(rect)
				box = np.intp(box)

				# draw a bounding box around the detected barcode
				cv2.drawContours(image, [box], -1, (0, 255, 0), 3)

			#############################################################
			# This end the part that was taken.


				detected_boxes.append(box)
			else:
				detected_boxes.append([0]) 
			# Output the final image and measure time it took to detect the entire image
			cv2.imwrite(dst_dir + img_name, image)
			end_time = time.time()
			cycle_time = end_time - start_time
			cycle_times.append(cycle_time)
			# Calculate all values of intersection over union for this image and store with ious of all images
			iou = metrics.calculate_ious(detected_boxes, gt_boxes)
			for i in iou:
				ious.append(i)
# Calculate and print average time it took to detect a bounding box and calculate and print various metrics for individual thresholds
avg_cycle_time = sum(cycle_times) / (len(ious) + np.finfo(np.float64).eps)
ap, recall, precision, f1 = metrics.calculate_AP(ious, 0.5)
print("AP@0.5: " + str(ap) + " recall: " + str(recall) + " precision: " + str(precision) + " F1: " + str(f1))
ap, recall, precision, f1 = metrics.calculate_AP(ious, 0.75)
print("AP@0.75: " + str(ap) + " recall: " + str(recall) + " precision: " + str(precision) + " F1: " + str(f1))
ap, recall, precision, f1 = metrics.calculate_AP(ious, 0.9)
print("AP@0.9: " + str(ap) + " recall: " + str(recall) + " precision: " + str(precision) + " F1: " + str(f1))
print("Average time to detect bounding box: " + str(avg_cycle_time))
