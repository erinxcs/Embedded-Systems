r"""ERINX V4.9 - Convert your images to 320x480 baseline JPEG for ESP32-S3.
Requires: python -m pip install pillow
Run: python convert_photos.py "C:\Users\you\Pictures" "E:\photos"
The destination must be your SD card's /photos folder or a staging folder.
"""
from pathlib import Path
from PIL import Image, ImageOps
import sys

if len(sys.argv) != 3:
    print('Usage: python convert_photos.py "INPUT_FOLDER" "OUTPUT_FOLDER"')
    sys.exit(1)
source = Path(sys.argv[1])
target = Path(sys.argv[2])
target.mkdir(parents=True, exist_ok=True)
exts={'.jpg','.jpeg','.png','.webp','.bmp','.tiff'}
count=0
for input_file in sorted(source.iterdir()):
    if not input_file.is_file() or input_file.suffix.lower() not in exts:
        continue
    try:
        with Image.open(input_file) as raw:
            im=ImageOps.exif_transpose(raw).convert('RGB')
            # Keep whole photo and letterbox it; don't crop faces/important areas.
            fitted=ImageOps.contain(im,(320,480),Image.Resampling.LANCZOS)
            canvas=Image.new('RGB',(320,480),(7,18,39))
            canvas.paste(fitted,((320-fitted.width)//2,(480-fitted.height)//2))
            filename=f'PIC_{count+1:03d}.jpg'
            canvas.save(target/filename,format='JPEG',quality=88,optimize=True,progressive=False,subsampling=0)
            print(input_file.name,'->',filename)
            count+=1
    except Exception as exc:
        print('SKIPPED',input_file,exc)
print('Converted',count,'photos. Reinsert microSD in ESP32-S3 when power is OFF.')
