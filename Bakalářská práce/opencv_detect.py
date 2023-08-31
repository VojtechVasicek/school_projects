import cv2
import os
import time
import sys
import numpy as np
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
decoded = 0
cycle_times = []
# Cycle through all images in given directory
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
			# This starts the part that was taken from https://note.nkmk.me/en/python-opencv-barcode/
			#########################################################################################
			# Detect and decode all barcodes in image using opencv detector
			bd = cv2.barcode.BarcodeDetector()
			retval, decoded_info, decoded_type, points = bd.detectAndDecode(img)
            # If none were found, do nothing
			if(isinstance(points, type(None))):
				pass
			# Otherwise insert the bounding box detected by opencv detector
			else:
				img = cv2.polylines(img, points.astype(int), True, (0, 255, 0), 3)
				for s, p in zip(decoded_info, points):
					img = cv2.putText(img, s, p[1].astype(int),
					cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1, cv2.LINE_AA)
			########################################################################################
			# This end the taken part.
                    
                    # If barcode was also decode, add 1 to total decoded barcodes
					if(s != 0):
						decoded += 1 
				
				for p in points:
					detected_boxes.append(p)
		# Output the final image and measure time it took to detect the entire image
		cv2.imwrite(dst_dir + img_name, img)
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
print("Decoded: " + str(decoded) + "/" + str(len(ious)))
print("Average time to detect bounding box: " + str(avg_cycle_time))

