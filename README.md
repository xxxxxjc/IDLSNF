<div align="center">
 <br>
<h1>Large Capacity H.265/HEVC Video Steganography Based on Polygon Encoding and Improved Deep Learnable Similarity Network</h1>
</div>

## ⏳ Quick Start
### 1. Description
```
HM 16.15 integrates IDLSNF by invoking cnndemo to replace the HEVC in-loop filter.
```
### 2.Getting datasets
| Datasets          |    Paper                                                                                                               |    Url    |
|:------:           |:---------:                                                                                                             |:---------:|
| DIV2K  | Global Learnable Attention for SingleImage Super-Resolution (TPAMI 2023)                                                          |  [[Google Drive](https://data.vision.ee.ethz.ch/cvl/DIV2K/)]|

### 3.Inference
Of course, you need to change [DetectionTests] in test.py when testing.

We present our inference results in log_test.log.
```
python test.py --model_path ./checkpoints/model_epoch_last.pth
```

## ⏳ Training
The training set uses four classes from CNN-Spot(CNN-generated images are surprisingly easy to spot...for now, CVPR 2020): car, cat, chair, and horse. [Baidu Netdisk](https://pan.baidu.com/s/1l-rXoVhoc8xJDl20Cdwy4Q?pwd=ft8b)
```
python train.py --name 4class-car-cat-chair-horse --dataroot [training datasets path] --classes car,cat,chair,horse
```
## Contact
If you have any question about this project, please feel free to contact 247918horizon@gmail.com





