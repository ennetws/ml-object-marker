:: Create positive samples
opencv_createsamples.exe -info data/sample_seg.txt -vec data/positives.vec -num 1000 -w 40 -h 60
opencv_createsamples.exe -vec data/positives.vec -num 1000 -w 40 -h 60