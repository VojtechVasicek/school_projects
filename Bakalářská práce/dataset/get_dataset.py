import json
from PIL import Image
import requests
from io import BytesIO
import numpy as np
import shutil
import os
import sys

# Creates root folder and all specified subfolders
def create_folders(root_folder, sub_folders):
    os.makedirs(root_folder, exist_ok=True)
    for folder_name in sub_folders:
        folder_path = os.path.join(root_folder, folder_name)
        os.makedirs(folder_path, exist_ok=True)

# Moves files specified in file_list_file from source_dir to target_dir.
# Can also change file extension to new_ext.
def move_img(source_dir, target_dir, file_list_file, old_ext, new_ext):
    with open(file_list_file, "r") as f:
        file_names = [line.strip() for line in f]

    for file_name in file_names:
        old_path = os.path.join(source_dir, file_name.split('.', 1)[0] + new_ext)
        new_path = os.path.join(target_dir, file_name.split('.', 1)[0] + new_ext)
        shutil.move(old_path, new_path)

# This function returns bounding box in the format required for YOLOv5 training [class, x_center, y_center, box_width, box_height]
def get_bbox(bbox, img):
    img_width, img_height = get_image_size(img)
    # Get all x and y values
    x_values = [point['x'] for point in bbox]
    y_values = [point['y'] for point in bbox]

    # Get the smallest and highest x and y values, to represent corners of non oriented bounding box
    x_min, y_min = min(x_values), min(y_values)
    x_max, y_max = max(x_values), max(y_values)
    # Get the width, height, x_center and y_center
    width = x_max - x_min
    height = y_max - y_min
    x_center = (x_min + (width / 2)) / img_width
    y_center = (y_min + (height / 2)) / img_height
    return "0 " + str(x_center) + " " + str(y_center) + " " + str(width/img_width) + " " + str(height/img_height)

# This function returns oriented bounding box
def get_oriented_bbox(bbox, img):
    # Find the centroid of the points
    bbox = np.array([(point['x'], point['y']) for point in bbox])

    # Compute the centroid of the point cloud
    centroid = np.mean(bbox, axis=0)

    # Compute the covariance matrix of the point cloud
    covariance_matrix = np.cov(bbox.T)

    # Find the eigenvectors and eigenvalues of the covariance matrix
    eigenvalues, eigenvectors = np.linalg.eig(covariance_matrix)

    # Sort the eigenvectors by descending eigenvalue
    sorted_indices = np.argsort(eigenvalues)[::-1]
    sorted_eigenvectors = eigenvectors[:, sorted_indices]

    # Rotate the point cloud so that the principal axes align with the x and y axes
    rotated_points = np.dot(bbox - centroid, sorted_eigenvectors)

    # Find the minimum and maximum x and y values along the rotated axes
    min_x = np.min(rotated_points[:, 0])
    max_x = np.max(rotated_points[:, 0])
    min_y = np.min(rotated_points[:, 1])
    max_y = np.max(rotated_points[:, 1])
    width, height = get_image_size(img)

    # Get the corners of the oriented bounding box in the rotated coordinate system
    corners = np.array([[min_x, min_y], [max_x, min_y], [max_x, max_y], [min_x, max_y]])

    # Rotate the corners back to the original orientation and translate them back to the original position
    unrotated_corners = np.dot(corners, sorted_eigenvectors.T) + centroid

    # Convert the corners to a string with spaces between each coordinate
    corners_string = ""
    for corner in unrotated_corners:
        if corner[0] < 0:
            corner[0] = 0
        if corner[1] < 0:
            corner[1] = 0
        if corner[0] > width:
            corner[0] = width
        if corner[1] > height:
            corner[1] = height
        corners_string += str(corner[0]) + " " + str(corner[1]) + " "
    return corners_string.strip()

# Returns image size
def get_image_size(response):
    img = Image.open(BytesIO(response.content))
    return img.size    

# Outputs the downloaded images and labels to named files
def append_to_file(geometry, id, url, dir, oriented):
    img = requests.get(url)
    with open("./images/" + dir + "/" + id + ".jpg", 'wb') as f:
        f.write(img.content)
    with open("./labels/" + dir + "/" + id + ".txt", "w") as f:
        for geo in geometry:
            if(oriented == "oriented"):
                f.write(get_oriented_bbox(geo['geometry'], img) + "\n")
            else:
                f.write(get_bbox(geo['geometry'], img) + "\n")



if len(sys.argv) != 2:
	print("This script takes 1 argument.")
	exit()
type = sys.argv[1]
# Load data from json
with open('export-2022-09-19T13_59_32.700Z.json') as f:
    data = json.load(f)
    i = 1
    # Decite how to split the dataset based on type
    if(type == "aliased"):
        create_folders("images", ["aliased_barcode", "barcode"])
        create_folders("labels", ["aliased_barcode", "barcode"])
        for line in data:
            if(line["Label"] != "Skip"):
                try:
                    append_to_file(line['Label']['aliased barcode'], line['ID'], line["Labeled Data"], "aliased_barcode", "not_oriented")
                except KeyError:
                    append_to_file(line['Label']['barcode'], line['ID'], line["Labeled Data"], "barcode", "not_oriented")
            print("Downloaded " + str(i) + "/" + str(len(data)) + " images")
            i+=1
    elif(type == "aliased_oriented"):
        create_folders("images", ["aliased_barcode", "barcode"])
        create_folders("labels", ["aliased_barcode", "barcode"])
        for line in data:
            if(line["Label"] != "Skip"):
                try:
                    append_to_file(line['Label']['aliased barcode'], line['ID'], line["Labeled Data"], "aliased_barcode", "oriented")
                except KeyError:
                    append_to_file(line['Label']['barcode'], line['ID'], line["Labeled Data"], "barcode", "oriented")
            print("Downloaded " + str(i) + "/" + str(len(data)) + " images")
            i+=1
    elif(type == "train"):
        create_folders("images", ["train", "val", "test"])
        create_folders("labels", ["train", "val", "test"])
        for line in data:
            if(line["Label"] != "Skip"):
                try:
                    append_to_file(line['Label']['aliased barcode'], line['ID'], line["Labeled Data"], "train", "not_oriented")
                except KeyError:
                    append_to_file(line['Label']['barcode'], line['ID'], line["Labeled Data"], "train", "not_oriented")
            print("Downloaded " + str(i) + "/" + str(len(data)) + " images")
            i+=1
        
        move_img("./images/train", "./images/val", "./logfile_val.txt", ".jpg")
        move_img("./images/train", "./images/test", "./logfile_test.txt", ".jpg")
        move_img("./labels/train", "./labels/val", "./logfile_val.txt", ".txt")
        move_img("./labels/train", "./labels/test", "./logfile_test.txt", ".txt")

    elif(type == "train_oriented"):
        create_folders("images", ["train", "val", "test"])
        create_folders("labels", ["train", "val", "test"])
        for line in data:
            if(line["Label"] != "Skip"):
                try:
                    append_to_file(line['Label']['aliased barcode'], line['ID'], line["Labeled Data"], "train", "oriented")
                except KeyError:
                    append_to_file(line['Label']['barcode'], line['ID'], line["Labeled Data"], "train", "oriented")
            print("Downloaded " + str(i) + "/" + str(len(data)) + " images")
            i+=1
        
        move_img("./images/train", "./images/val", "./logfile_val.txt", ".jpg")
        move_img("./images/train", "./images/test", "./logfile_test.txt", ".jpg")
        move_img("./labels/train", "./labels/val", "./logfile_val.txt", ".txt")
        move_img("./labels/train", "./labels/test", "./logfile_test.txt", ".txt")
    else:
        print("Unknown dataset split.")
