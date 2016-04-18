# RandomGenTree
# authors: Jasjot S Sumal, Amador J Silva
Final Project for CS371 (Computer Graphics), Fall 2014

following code contributed by instructor, Gordon Griesel:
  fonts.h, libggfonts.so, libggfonts32.so, log.c, log.h,
  and some code in tree_3d.c for opening/resizing window, tracking mouse, and initializing opengl

Warning:  This project usually takes a lot of computational resources,
          and is not using optimal algorithms, so program may sometimes perform slowly.

To run project (on Linux operating systems):
  1. open the command line prompt and navigate to the directory containing the project
  
if the computer is using 64-bit architechture:
  2. to compile the executable, enter the following in the prompt:
    >  make tree_3d
  3. then to run the program:
    >  ./tree_3d
    
if the computer is using 32-bit architechture:
  2. to compile the executable, enter the following in the prompt:
    >  make tree_3d_32bit
  3. then to run the program:
    >  ./tree_3d
    
  4. to remove the executable and any associated assembly files, use:
    >  make clean
    
Key Mappings:
0           - show menu
1           - generate a new tree
x, y, z     - rotate tree on x, y, or z axis
w, a, s d   - translate tree up(w), left(a), down(s), or right(d)
8, 9        - zoom in/out
    
Troubleshooting (known issues):
1. If the 'make' command is giving problems with the font system being used,
   these fonts can be removed from the program using the following command
   (must be in the project directory):
  >  make rem_fonts
  
  To undo the above command, use:
  >  make add_fonts
