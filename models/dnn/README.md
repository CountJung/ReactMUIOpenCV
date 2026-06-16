# Optional DNN Model Assets

This folder is intentionally empty in Git. Put large OpenCV DNN model files here when you want to run Image Lab or Pipeline Flow DNN examples. The app can also download selected public model packages from Image Lab.

The backend only accepts relative paths under `models/dnn`. Absolute paths and `..` traversal are rejected.

## Typical Layouts

```txt
models/dnn/
├─ face/
│  ├─ opencv_face_detector.prototxt
│  └─ opencv_face_detector.caffemodel
├─ yolo/
│  ├─ yolo11n.onnx
│  └─ coco.names
├─ text/
│  └─ frozen_east_text_detection.pb
├─ pose/
│  ├─ pose_deploy_linevec.prototxt
│  └─ pose_iter_440000.caffemodel
└─ mask-rcnn/
   ├─ frozen_inference_graph.pb
   ├─ mask_rcnn_inception_v2_coco_2018_01_28.pbtxt
   └─ coco.names
```

## Image Lab Steps

1. Start the app and open or upload an image in Image Lab.
2. In `Optional DNN Examples`, choose a package under `Downloadable Model Packages`.
3. Click `Download`. The backend downloads only catalog-approved URLs into `models/dnn`.
4. After download, click `Use`; the operation and relative paths are filled automatically.
5. Tune confidence, input size, scale, mean, and `Swap RB`.
6. Click `Apply`.

Manual setup still works: copy model files into a subfolder under `models/dnn`, then type relative paths such as `yolo/yolo11n.onnx` and `yolo/coco.names`.

## Downloadable Packages

The built-in catalog currently includes:

- `opencv-face-ssd`: OpenCV ResNet-10 SSD face detector.
- `yolo11n-coco`: Ultralytics YOLO11n ONNX model plus COCO labels.
- `east-text-detector`: EAST scene text detector frozen TensorFlow graph.
- `openpose-coco`: OpenPose COCO pose model and OpenCV config.
- `mask-rcnn-coco`: Mask R-CNN COCO TensorFlow graph, config, and labels.

Large assets remain ignored by Git. Review each upstream source and license before redistributing downloaded models.

If a model is missing or the path is outside `models/dnn`, the request fails with a guarded error instead of reading arbitrary files.
