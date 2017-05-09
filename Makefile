
# This is the Makefile for OpenCV projects in EEET2448, Centre of Technology RMIT Vietnam
# Last revision:    13 March 2017
	
# These are the folders where #include<header> will look for the header files
INCLUDES = -I"C:\Users\HP\workspace\opencv\build\include" -I"C:\Users\HP\workspace\opencv\build\include\opencv" -I"C:\Users\HP\workspace\opencv\build\include\opencv2"

# The include folders have to added when compiling the C++ source codes,
 # thus the flag "$(INCLUDES)"
CXXFLAGS =	-O2 -g -Wall -fmessage-length=0 $(INCLUDES)

# To be safe, link all OpenCV libraries during compilation
# Otherwise, you might get several annoying "undefined reference" errors
# The library opencv_imgcodecs310 is needed by imread()
LIBS = -lopencv_core310.dll -lopencv_imgproc310.dll -lopencv_highgui310.dll \
-lopencv_ml310.dll -lopencv_video310.dll -lopencv_videoio310.dll -lopencv_features2d310.dll \
-lopencv_calib3d310.dll -lopencv_objdetect310.dll -lopencv_flann310.dll \
-lopencv_imgcodecs310.dll

LIBPATH =  -L"C:\Users\HP\workspace\opencv\build\x64\mingw_1\lib"

# At runtime, the program will look for the DLL files.
# Make sure the folder of the DLL files are in the System Environment Path:
# Start > Right click Computer > Properties > Advanced System Settings
#       > Environment Variables > System Variables > Edit Path
#       > Add a semicolon and the name of the folder
#     that contains the following DLL files:
#
# libopencv_core310.dll libopencv_imgproc310.dll libopencv_highgui310.dll
# libopencv_ml310.dll libopencv_video310.dll libopencv_videoio310.dll
# libopencv_features2d310.dll libopencv_calib3d310.dll libopencv_objdetect310.dll
# libopencv_flann310.dll libopencv_imgcodecs310.dll

OBJS = boxing_game.o

TARGET = boxing_game.exe

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBPATH) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)


