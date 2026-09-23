from alpr_pipeline import Pipeline

new_pipeline = Pipeline(model="../best50_ncnn_model");

print(f"The license plate captured is: {new_pipeline.take_picture()}")