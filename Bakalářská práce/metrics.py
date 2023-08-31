import numpy as np
import os
from PIL import Image
from shapely.geometry import Polygon


# If file is an image, return true, otherwise return false
def is_image(filename):
    try:
        img = Image.open(filename)
        img.verify()
        return True
    except:
        return False

# If path to folder to store results doesnt exist, create it
# If path to other folders doesnt exist, print message and end the script
# If the path exists and doesnt end with '/', add it to the end of the path
# Returns the argument
def check_arg(arg, type):
    if type == "result":
        if os.path.exists(arg) == False:
            os.makedirs(arg, exist_ok=True)
    else:
        if os.path.exists(arg) == False:
            print("Path to " + type + " doesnt exist.")
            exit()
    if arg[-1] != "/":
         arg+="/"
    return arg

# Transforms file to array, each line is its own array within an array
# Each element in the line array is a value split by space
# Returns an array of arrays
def get_boxes(file_path):
    file_path = change_file_format(file_path, 'txt')
    array_list = []
    with open(file_path, 'r') as f:
        for line in f:
            word_array = line.strip().split(' ')
            array_list.append(word_array)
    return array_list

# Changes the extension of file to the new_format file extension
# Returns filepath with changed file extension
def change_file_format(file_path, new_format):
    # Get the base file name and original file extension
    base_name, orig_extension = os.path.splitext(file_path)
    
    # Create the new file name with the specified format
    new_file_name = base_name + '.' + new_format
    
    return new_file_name

# Transforms bounding box from absolute coordinates to relative coordinates
# Returns the transformed bounding box
def transform_bounding_box(bbox, image_path):
    with Image.open(image_path) as img:
        width, height = img.size
    image_size = np.array([width, height]).reshape(1, 2)
    bbox_relative = bbox / image_size
    return bbox_relative

# Takes bounding box in format [x1, y1, x2, y2, x3, y3, x4, y4] and transforms it
# to [[x1, y1], [x2, y2], [x3, y3], [x4, y4]] to represent the corners individualy
def get_corners(box):
    if(len(box) == 1):
        return np.zeros((4, 2))
    x = box[0::2]
    y = box[1::2]
    corners = np.zeros((4, 2))
    corners[0] = [x[0], y[0]]
    corners[1] = [x[1], y[1]]
    corners[2] = [x[2], y[2]]
    corners[3] = [x[3], y[3]]
    return corners

# Calculates intersection area between two bounding boxes and returns it as float
# If there is no intersection returns 0.0
def get_intersection_area(cornersA, cornersB):

    poly1 = Polygon(cornersA)
    poly2 = Polygon(cornersB)
    if(poly1.intersects(poly2)):
        intersection = poly1.intersection(poly2)
        return intersection.area
    else:
        return 0.0

# Calculates the union area between two bounding boxes and returns it as float
def get_union_area(cornersA, cornersB):
    poly1 = Polygon(cornersA)
    poly2 = Polygon(cornersB)
    union = poly1.union(poly2)
    return union.area


# Calculates intersection over union between all bounding boxes in one image and returns
# the biggest values. The number of returned values is the number of detected bounding boxes.
# If the number of detected boxes is smaller than the number of ground truth boxes, append
# -1 until the number is same.
def calculate_ious(detected_boxes, gt_boxes):
	ious = []
	for boxB in gt_boxes:
		if(len(detected_boxes) == 0):
			break
		else:
			boxB = np.asarray(boxB, dtype=np.float32)
			for boxA in detected_boxes:
				boxA = np.array(boxA)
				boxA = boxA.flatten()
				cornersA = get_corners(boxA)
				
				cornersB = get_corners(boxB)

				interArea = get_intersection_area(cornersA, cornersB)
				unionArea = get_union_area(cornersA, cornersB)

				ious.append(interArea / unionArea)
	
	indices = np.argsort(ious)[-len(detected_boxes):]
	biggest_values = []
	for i in indices:
		biggest_values.append(ious[i])
	if(len(detected_boxes) < len(gt_boxes)):
		for j in range(len(gt_boxes) - len(detected_boxes)):
			biggest_values.append(-1)
	return biggest_values

# Converts a bounding box in [class, x_center, y_center, width, height] format to [[x1, y1],[x1, y2],[x2, y2],[x2, y1]] format
def bbox_to_points(bbox):
    if(len(bbox) == 5):
        type, x_center, y_center, width, height = bbox
    else:
        print("The format of YOLOv5 bounding boxes is wrong.")
        exit()
    x1 = abs(float(x_center) - float(width) / 2)
    y1 = abs(float(y_center) - float(height) / 2)
    x2 = abs(float(x_center) + float(width) / 2)
    y2 = abs(float(y_center) + float(height) / 2)
    return [[x1, y1],[x1, y2],[x2, y2],[x2, y1]]

# Calculates the number of false negatives, true positives and false positives based on the threshold
# The function then calculates precision and recall using these numbers. Then it calculates the F1 score
# and average precision using precision and recall. The np.finfo values are the lowest posible float values.
# They are there to ensure division by 0 is not possible.
def calculate_AP(ious, iou_threshold):
    true_positives = 0
    false_positives = 0
    false_negatives = 0
    for iou in ious:
        if iou == -1:
            false_negatives += 1
        elif iou > iou_threshold:
            true_positives += 1
        else:   
            false_positives += 1

    precision = true_positives / (true_positives + false_positives + np.finfo(np.float64).eps)
    recall = true_positives / (true_positives + false_negatives + np.finfo(np.float64).eps)
    f1 = 2 * ((precision * recall) / (precision + recall + np.finfo(np.float64).eps))
    ap = 0
    for t in np.arange(0, 1.1, 0.1):
        if np.sum(recall >= t) == 0:
            p = 0
        else:
            p = np.max(precision[recall >= t])
        ap = ap + p / 11
    return ap, recall, precision, f1

# Transforms YOLOv5 bounding box from relative values to absolute values
def relative_to_absolute_coords(coords, image_width, image_height):
	coords = [float(x) for x in coords]
	bbox_type, x_center, y_center, width, height = coords
	
	x = int(coords[1] * image_width)
	y = int(coords[2] * image_height)
	w = int(coords[3] * image_width)
	h = int(coords[4] * image_height)
	
	return bbox_type,x, y, w, h
