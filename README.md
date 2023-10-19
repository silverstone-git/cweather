# cweather
An Open Weather Map Client App in C

A cli tool to get weather output from Open Weather Map using curl

- Auto detects location if no input
- Works with the name of the location as well

# Prerequisites
- [cjson](https://github.com/DaveGamble/cJSON)
- [libcurl](https://curl.se/libcurl/)
Now cd into a new folder 'cweather_v1_5'

# Installation
- copy openweathermap API key from [website](https://openweathermap.org/current) and copy it into a text file, save as owmapi.key
- build the main.c using gcc
> gcc -l cjson -l curl main.c -o cweather

# Usage
Search by Location name
> cweather "Your location name here in strings"
Search by coordinates
> cweather 0.1, 0.1
Auto Detect by IP
> cweather
