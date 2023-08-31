import os
import cv2
from PIL import Image
import numpy as np
import time
import sys
import metrics


# Check if all arguments are valid
if len(sys.argv) != 5:
	print("This script takes 4 arguments.")
	exit()
img_dir = metrics.check_arg(sys.argv[1], "images")
label_dir = metrics.check_arg(sys.argv[2], "labels")
yolo_dir = metrics.check_arg(sys.argv[4], "YOLOv5")
dst_dir = metrics.check_arg(sys.argv[3], "result")

# Measure time it takes YOLOv5 to run the complete detection of all images
start_time1 = time.time()
os.system("python3 " + yolo_dir + "detect.py --weights " + yolo_dir + "best.pt --source '" + img_dir + "*' --save-txt --save-crop")
end_time1 = time.time()
# Take last run of YOLOv5 detection
yolo_run = os.listdir(yolo_dir + "runs/detect/")[-1]
dir_list = os.listdir(img_dir)
all_ious = []
cycle_times = []
decoded = 0
# Cycle through all images in given directory
for img_name in dir_list:
	# Check if provided file is image, if not, the file gets skipped
	if(metrics.is_image(img_dir + img_name)):
		start_time2 = time.time()
		img = cv2.imread(img_dir + img_name)
		detected_boxes = []
		# Load bounding boxes detected by YOLOv5 as yolo_boxes and ground truth bounding boxes as gt_boxes
		gt_boxes = metrics.get_boxes(metrics.change_file_format(label_dir + img_name, "txt"))
		yolo_boxes = metrics.get_boxes(metrics.change_file_format(yolo_dir + "runs/detect/" + yolo_run + "/labels/" + img_name, "txt"))
		i = 1
		# Cycle through all bounding boxes detected by YOLOv5
		for yolo_box in yolo_boxes:
			if(img is not None):
				# Change names for cropped bounding box
				if(i > 1):
					tmp = img_name.split(".")
					tmp[0] += str(i) + "."
					image_name = tmp[0] + tmp[1]
				else:
					image_name = img_name
				# Check if cropped bounding box exists
				if os.path.isfile(yolo_dir + "runs/detect/" + yolo_run + "/crops/barcode/" + image_name):
					img_crop = cv2.imread(yolo_dir + "runs/detect/" + yolo_run + "/crops/barcode/" + image_name)
					img_crop = cv2.GaussianBlur(img_crop, (3,3), 0)
					image_height, image_width = img.shape[:2]
					# Create empty image and insert the cropped bounding box to the coordinates detected in original image
					empty_img = Image.new("RGB", (image_width, image_height), color=(255,255,255))
					insert_image = Image.fromarray(cv2.cvtColor(img_crop, cv2.COLOR_BGR2RGB))
					yolo_box = metrics.relative_to_absolute_coords(yolo_box, image_width, image_height)
					top_left_x = int(yolo_box[1] - (yolo_box[3] / 2))
					top_left_y = int(yolo_box[2] - (yolo_box[4] / 2))
					empty_img.paste(insert_image, (top_left_x, top_left_y))

					# This part below was taken from https://note.nkmk.me/en/python-opencv-barcode/
					####################################################################################
					# Detect and decode all barcodes in image using opencv detector
					bd = cv2.barcode.BarcodeDetector()
					retval, decoded_info, decoded_type, points = bd.detectAndDecode(np.array(empty_img))
					####################################################################################


					# If bounding box wasnt detected by opencv detector, insert the one detected by YOLOv5
					if(isinstance(points, type(None))):
						yolo_box = np.int32(np.array([metrics.bbox_to_points(yolo_box)]))
					# This part below was taken from https://note.nkmk.me/en/python-opencv-barcode/
					####################################################################################
						img = cv2.polylines(img, yolo_box.astype(int), True, (0, 255, 0), 3)
						for s, p in zip(decoded_info, yolo_box):
							img = cv2.putText(img, s, p[1].astype(int),
							cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1, cv2.LINE_AA)
					####################################################################################

							# If barcode was also decode, add 1 to total decoded barcodes
							if(s != 0):
								decoded += 1
						for p in yolo_box:
							detected_boxes.append(p)
					# Insert the bounding box detected by opencv detector
					else:
					# This part below was taken from https://note.nkmk.me/en/python-opencv-barcode/
					####################################################################################
						img = cv2.polylines(img, points.astype(int), True, (0, 255, 0), 3)
						for s, p in zip(decoded_info, points):
							img = cv2.putText(img, s, p[1].astype(int),
							cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1, cv2.LINE_AA)
					####################################################################################
							if(s != 0):
								decoded += 1
						for p in points:
							detected_boxes.append(p)
				i+=1
		# Output the final image and measure time it took to detect the entire image	
		cv2.imwrite(dst_dir + img_name, img)
		end_time2 = time.time()
		cycle_time = end_time2 - start_time2
		cycle_times.append(cycle_time)
		# Calculate all values of intersection over union for this image and store with ious of all images
		ious = metrics.calculate_ious(detected_boxes, gt_boxes)
		for iou in ious:
			all_ious.append(iou)
# Calculate and print average time it took to detect a bounding box and calculate and print various metrics for individual thresholds
avg_cycle_time = (sum(cycle_times) + (end_time1 - start_time1)) / (len(all_ious) + np.finfo(np.float64).eps)
ap, recall, precision, f1 = metrics.calculate_AP(all_ious, 0.5)
print("AP@0.5: " + str(ap) + " recall: " + str(recall) + " precision: " + str(precision) + " F1: " + str(f1))
ap, recall, precision, f1 = metrics.calculate_AP(all_ious, 0.75)
print("AP@0.75: " + str(ap) + " recall: " + str(recall) + " precision: " + str(precision) + " F1: " + str(f1))
ap, recall, precision, f1 = metrics.calculate_AP(all_ious, 0.9)
print("AP@0.9: " + str(ap) + " recall: " + str(recall) + " precision: " + str(precision) + " F1: " + str(f1))
print("Decoded: " + str(decoded) + "/" + str(len(all_ious)))
print("Average time to detect bounding box: " + str(avg_cycle_time))
