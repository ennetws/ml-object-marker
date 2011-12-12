:: Create negative samples
opencv_createsamples.exe -info negatives/negatives.txt -vec negatives/negatives.vec -w 20 -h 30 -num 2000
opencv_createsamples.exe -vec negatives/negatives.vec -w 20 -h 30 -show 1.0 -num 2000