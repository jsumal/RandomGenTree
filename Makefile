NAME4 = tree_3d
DEP1 = vecFxns.c
DEP2 = log.c
DEP3 = importObj.c
CFLAGS = -Wall -Wextra -g

all: $(NAME4)

$(NAME4): $(NAME4).c $(DEP1) $(DEP2) $(DEP3)
	gcc $(NAME4).c $(DEP1) $(DEP2) $(DEP3) $(CFLAGS) -o $(NAME4) -lX11 -lGL -lGLU -lm ./libggfonts.so

$(NAME4)_32bit: $(NAME4).c $(DEP1) $(DEP2) $(DEP3)
	gcc $(NAME4).c $(DEP1) $(DEP2) $(DEP3) $(CFLAGS) -o $(NAME4) -lX11 -lGL -lGLU -lm ./libggfonts32.so

clean:
	rm -f $(NAME4)
	rm -f *.o

rem_fonts:
	sed -i 's/ initialize_fonts(/ \/\/initialize_fonts(/' $(NAME4).c
	sed -i 's/ cleanup_fonts(/ \/\/cleanup_fonts(/' $(NAME4).c
	sed -i 's/ ggprint/ \/\/ggprint/' $(NAME4).c
	sed -i 's/ Rect / \/\/Rect /' $(NAME4).c
	sed -i 's/ r\./ \/\/r./' $(NAME4).c

add_fonts:
	sed -i 's/ \/\/initialize_fonts(/ initialize_fonts(/' $(NAME4).c
	sed -i 's/ \/\/cleanup_fonts(/ cleanup_fonts(/' $(NAME4).c
	sed -i 's/ \/\/ggprint/ ggprint/' $(NAME4).c
	sed -i 's/ \/\/Rect / Rect /' $(NAME4).c
	sed -i 's/ \/\/r\./ r./' $(NAME4).c
