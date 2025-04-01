# GPX-Track-Visualizer
![image](https://github.com/user-attachments/assets/9c9319fc-4d91-4af5-9260-a4f06e0fe542)
![image](https://github.com/user-attachments/assets/074132eb-f35d-400d-9d8e-f4eb00e45fd8)

# How to Use:
### Installation
Make sure [tinyxml2](https://github.com/leethomason/tinyxml2) is installed. If it isn't itstall it with:
```
git clone https://github.com/leethomason/tinyxml2.git
cd tinyxml2
make
sudo make install
```
Then compile the code:
```
chmod +x compile\ plot.cpp.sh && ./compile\ plot.cpp.sh
```
### Run
```
./gpx_visualizer /path/to/.gpx/files
```
# Hotkeys:
```
P:       Lines / Points
S:       Speed
Up/Down: Change line distance threshold
Scroll:  Zoom
LMB:     Move
```
# Color
(for coordinates that are taken every 30s, change code for different intervals.)
```
Dark blue:   0km/h
Light Blue:  5km/h
Green:      10km/h
Yellow:     20km/h
Orange:     30km/h
Red:        50km/h
```
