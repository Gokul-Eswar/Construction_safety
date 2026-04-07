from ultralytics import YOLO

def main():
    print("Loading yolov8n.pt...")
    model = YOLO('yolov8n.pt')
    print("Exporting to ONNX for OpenCV DNN compatibility...")
    # OpenCV DNN supports YOLOv8 well.
    model.export(format='onnx', opset=12, simplify=True, dynamic=False)
    print("Export complete. Ensure the new 'yolov8n.onnx' is in the root directory.")

if __name__ == '__main__':
    main()
