import onnx
from onnxsim import simplify
import os
from util import CURRENTPATH
import onnx2tf

model_name = 'epoch-28-acc-98.74213836477988-model.onnx'
filename = os.path.join(CURRENTPATH, model_name)
# load your predefined ONNX model
model = onnx.load(filename)

# convert model
model_simp, check = simplify(model)

assert check, "Simplified ONNX model could not be validated"

onnx.save(model_simp, os.path.join(CURRENTPATH, 'model_simp-' + model_name))

onnx2tf.convert(
    input_onnx_file_path=filename,
    output_folder_path=os.path.join(CURRENTPATH, "model.tf"),
    not_use_onnxsim=True
)
