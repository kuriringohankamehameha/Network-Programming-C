#!/bin/sh

for i in `seq 1 3`;
do
    notify-send -i ''"$HOME"'/Downloads/download_icon_color.png' 'Downloaded '
    sleep 4s
done
