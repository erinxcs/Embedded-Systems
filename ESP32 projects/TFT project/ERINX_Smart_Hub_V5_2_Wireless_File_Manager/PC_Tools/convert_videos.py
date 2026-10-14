#!/usr/bin/env python3
r"""Convert common videos to ERINX .evm (length-prefixed JPEG frames).

Required: Pillow and FFmpeg (available on PATH). Default: 240x360 @ 7 fps,
first 12 seconds only, silent, blue-black letterboxing, baseline JPEG.

Examples (PowerShell):
  python ./convert_videos.py .\MyVideos .\ConvertedVideos
  python ./convert_videos.py .\MyVideos .\ConvertedVideos --seconds 20 --fps 8
  python ./convert_videos.py .\MyVideos .\ConvertedVideos --width 320 --height 480
"""
from __future__ import annotations
import argparse
import io
import shutil
import struct
import subprocess
import sys
from pathlib import Path
from PIL import Image

VIDEO_TYPES = {".mp4", ".mov", ".avi", ".mkv", ".webm", ".m4v", ".gif"}
FRAME_MAX = 128 * 1024
HEADER = struct.Struct('<4sHHHHI')
LENGTH = struct.Struct('<I')


def convert_one(source: Path, target: Path, width: int, height: int, fps: int,
                seconds: int, quality: int) -> int:
    # Raw RGB24 pipe means Python/Pillow can make baseline JPEGs without
    # creating thousands of temporary JPG files on the PC.
    filter_str = (
        f"fps={fps},scale={width}:{height}:force_original_aspect_ratio=decrease:flags=lanczos,"
        f"pad={width}:{height}:(ow-iw)/2:(oh-ih)/2:color=0x0b193a"
    )
    command = ["ffmpeg", "-hide_banner", "-loglevel", "error", "-nostdin", "-i", str(source),
               "-t", str(seconds), "-an", "-sn", "-vf", filter_str,
               "-pix_fmt", "rgb24", "-f", "rawvideo", "pipe:1"]
    frame_size = width * height * 3
    frames = 0
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        with target.open('wb') as output:
            output.write(HEADER.pack(b'ERXV', width, height, fps, 0, 0))
            while True:
                data = process.stdout.read(frame_size)
                if not data:
                    break
                if len(data) != frame_size:
                    raise RuntimeError('Incomplete raw frame from FFmpeg')
                image = Image.frombytes('RGB', (width, height), data)
                bytesio = io.BytesIO()
                image.save(bytesio, format='JPEG', quality=quality, subsampling=2,
                           progressive=False, optimize=True)
                jpeg = bytesio.getvalue()
                if len(jpeg) > FRAME_MAX:
                    raise ValueError(f'Frame {frames + 1} uses {len(jpeg)} bytes; maximum {FRAME_MAX}. '
                                     'Lower quality/resolution.')
                output.write(LENGTH.pack(len(jpeg)))
                output.write(jpeg)
                frames += 1
            output.seek(0)
            output.write(HEADER.pack(b'ERXV', width, height, fps, 0, frames))
        error_bytes = process.stderr.read()
        result = process.wait()
        if result != 0 or frames == 0:
            message = error_bytes.decode('utf-8', errors='replace').strip()
            raise RuntimeError(message or 'FFmpeg returned no frames')
    except Exception:
        process.kill()
        process.wait()
        target.unlink(missing_ok=True)
        raise
    finally:
        if process.stdout: process.stdout.close()
        if process.stderr: process.stderr.close()
    return frames


def main() -> int:
    parser = argparse.ArgumentParser(description='ERINX Smart Hub MP4/MOV/etc -> .evm video converter')
    parser.add_argument('input', help='Folder of MP4/MOV/AVI/etc files or one video file')
    parser.add_argument('output', help='Output folder for .evm files')
    parser.add_argument('--fps', type=int, default=7, help='1 to 15, default 7')
    parser.add_argument('--seconds', type=int, default=12, help='Max seconds per clip (default 12)')
    parser.add_argument('--width', type=int, default=240, help='Portrait video width (default 240)')
    parser.add_argument('--height', type=int, default=360, help='Portrait video height (default 360)')
    parser.add_argument('--quality', type=int, default=58, help='JPEG quality, 20..85 (default 58)')
    args = parser.parse_args()
    if not shutil.which('ffmpeg'):
        print('ERROR: FFmpeg not found. On Windows run: winget install Gyan.FFmpeg', file=sys.stderr)
        print('Then restart PowerShell and check: ffmpeg -version', file=sys.stderr)
        return 2
    if not (1 <= args.fps <= 15 and 1 <= args.seconds <= 120 and
            64 <= args.width <= 320 and 64 <= args.height <= 480 and
            args.width % 8 == 0 and args.height % 8 == 0 and
            20 <= args.quality <= 85):
        parser.error('fps 1-15, seconds 1-120, width/height multiples of 8 with <=320x480, quality 20-85')
    source = Path(args.input).expanduser().resolve()
    destination = Path(args.output).expanduser().resolve()
    if not source.exists():
        parser.error(f'Not found: {source}')
    if source.is_dir():
        inputs = sorted(x for x in source.iterdir() if x.is_file() and x.suffix.lower() in VIDEO_TYPES)
    else:
        inputs = [source] if source.suffix.lower() in VIDEO_TYPES else []
    if not inputs:
        print(f'No supported video files found in {source}')
        return 1
    destination.mkdir(parents=True, exist_ok=True)
    errors = 0
    for idx, clip in enumerate(inputs, 1):
        target = destination / (clip.stem + '.evm')
        if target.exists():
            print(f'WARNING: overwrite existing file: {target}')
        try:
            count = convert_one(clip, target, args.width, args.height, args.fps,
                                args.seconds, args.quality)
            print(f'[{idx}/{len(inputs)}] {clip.name} -> {target.name} '
                  f'({count} frames, ~{count / args.fps:.1f}s, {target.stat().st_size//1024} KB)')
        except Exception as error:
            errors += 1
            print(f'FAILED {clip.name}: {error}', file=sys.stderr)
    print(f'Conversion complete. {len(inputs)-errors} success, {errors} failed.')
    return 1 if errors else 0


if __name__ == '__main__':
    raise SystemExit(main())
