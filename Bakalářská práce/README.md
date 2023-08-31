This folder contains:
- script requirements.py, that downloads all the required libraries needed to run this script on a computer
  in FIT CVT.

- 3 image detectors (merge_detect.py, opencv_detect.py, scharr_detect.py)

- python script metrics.py, which is called from within the detectors and contains functions
  to calculate various metrics

- 8 example cases to run the detectors on in dataset_examples folder

- trained neural network yolov5

- folder containing python script used to download the entire dataset(get_dataset.py), json containing
  sources for the dataset and two text files specifying the split used to train the yolov5 neural network

- folder containing the latex source code for the thesis

- pdf file of the thesis

- this README file

The scripts below use the following libraries: numpy, imutils, cv2, os, time, sys, PIL, shapely, io, requests,
json and shutil.


Script requirements.py

This script needs to be run before the rest. It install the required libraries, that are not present on computers
in FIT CVT. The required libraries are:
- opencv-contrib-python
- shapely
And libraries required by YOLOv5:
- ipython
- psutil
- torch torchvision
- GitPython

The script can be run as follows:
python3 requirements.py



Script get_dataset.py

Because the dataset is larger than the capacity of this DVD, it is not included. Instead, this script downloads the dataset.
This script takes one argument, representing the way the daset should be split and whether the labels should be oriented or not.
The 4 arguments to split the dataset are:

- aliased 		 - downloads not oriented dataset, that is split between aliased images and not aliased images
- aliased_oriented - downloads oriented dataset, that is split between aliased images and not aliased images
- train 		 - downloads not oriented dataset, that is split into train, val and test folders as used in training
	               of the YOLOv5 neural network
- train_oriented   - downloads oriented dataset, that is split into train, val and test folders as used in
                     training of the YOLOv5 neural network

Be careful, as with the train options the images and labels are moved to their proper directory after they are all
downloaded. Before that they are all in the train directory. The aliased split splits the as they are downloaded.
Example of calling the script is below:

python3 get_dataset.py aliased_oriented




Detectors scharr_detect.py and opencv_detect.py

These methods take 3 arguments to run. Below is represented the general command to run these scripts:

python3 [name_of_script] [path_to_images_to_detect] [path_to_ground_truth_labels] [path_to_output_location]

Below are examples to call each script:

python3 scharr_detect.py ./dataset_examples/images/ ./dataset_examples/labels/ ./results
python3 opencv_detect.py ./dataset_examples/images/ ./dataset_examples/labels/ ./results

The output images with detected bounding boxes will be located in ./results. The scripts will also
print out various metrics for various thresholds. Specificaly average precision, recall, precision
and F1 score for values of threshold 0.5, 0.75 and 0.9. They will also output average time to detect
a bounding box. Opencv detector will also output number of decode barcodes out of all detected barcodes.




Detector merge_detect.py

This method will take 4 arguments. The first 3 are the same as in the first two detectors and the last one
represents the location of yolov5 folder. Below is represented the general command to run this script:

python3 merge_detect.py [path_to_images_to_detect] [path_to_ground_truth_labels] [path_to_output_location] [path_to_yolo]

Below is an example call of the script:

python3 merge_detect.py ./dataset_examples/images/ ./dataset_examples/labels/ ./results ./yolov5/

The output images with detected bounding boxes will be located in ./results. The scripts will also
print out various metrics for various thresholds. Specificaly average precision, recall, precision
and F1 score for values of threshold 0.5, 0.75 and 0.9. They will also output average time to detect
a bounding box. Opencv detector will also output number of decode barcodes out of all detected barcodes.


