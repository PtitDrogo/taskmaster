OBJS_DIR	=	.objs
DEPS_DIR	=	.deps
SRCS_DIR	=	src
HEADER_DIR	=	include

NAME_D		=	taskmasterd
NAME_C		=	taskmasterctl
NAME		=	$(NAME_D) $(NAME_C)

#------------------------------------------------------------------------#

#---------------------------------Sources---------------------------------#
SRCS_COMMON	=	$(SRCS_DIR)/common/ini.cpp \
 				$(SRCS_DIR)/common/ProgramConfig.cpp 

SRCS_DAEMON	=	$(SRCS_DIR)/daemon/main.cpp \
				$(SRCS_DIR)/daemon/Server.cpp \
				$(SRCS_DIR)/daemon/dropPrivileges.cpp

SRCS_CLIENT	=	$(SRCS_DIR)/client/main.cpp \
				$(SRCS_DIR)/client/Client.cpp 

OBJS_COMMON	=	$(patsubst $(SRCS_DIR)/%.cpp, $(OBJS_DIR)/%.o, $(SRCS_COMMON))
OBJS_DAEMON	=	$(patsubst $(SRCS_DIR)/%.cpp, $(OBJS_DIR)/%.o, $(SRCS_DAEMON)) $(OBJS_COMMON)
OBJS_CLIENT	=	$(patsubst $(SRCS_DIR)/%.cpp, $(OBJS_DIR)/%.o, $(SRCS_CLIENT)) $(OBJS_COMMON)

DEPS		=	$(OBJS_DAEMON:.o=.d) $(OBJS_CLIENT:.o=.d)
DEPS		:=	$(sort $(DEPS))
#------------------------------------------------------------------------#

#---------------------------------Compilation & Linking---------------------------------#
CC		=	c++
RM		=	rm -f

CC_C		=	cc

CFLAGS		=	-Wall -Werror -Wextra -g3 -lreadline

INCLUDES	=	-I $(HEADER_DIR) -MMD -MP

#------------------------------------------------------------------------#


#---------------------------------Pretty---------------------------------#
YELLOW	=	\033[1;33m
GREEN	=	\033[1;32m
RESET	=	\033[0m
UP		=	"\033[A"
CUT		=	"\033[K"
#------------------------------------------------------------------------#


all: $(NAME)
-include $(DEPS)

FORCE:

#we need both a .c and .cpp rule, since our parsing ini library is in C.
$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.cpp
	@mkdir -p $(@D)
	@echo "$(YELLOW)Compiling [$<]$(RESET)"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ 
	@printf $(UP)$(CUT)

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.c
	@mkdir -p $(@D)
	@echo "$(YELLOW)Compiling [$<]$(RESET)"
	@$(CC_C) $(CFLAGS) $(INCLUDES) -c $< -o $@
	@printf $(UP)$(CUT)

$(NAME_D): $(OBJS_DAEMON) Makefile
	@echo "$(YELLOW)Linking [$@]$(RESET)"
	@$(CC) $(OBJS_DAEMON) $(CFLAGS) $(INCLUDES) -o $@ 
	@printf $(UP)$(CUT)
	@echo "$(GREEN)$(NAME_D) compiled!$(RESET)"

$(NAME_C): $(OBJS_CLIENT) Makefile
	@echo "$(YELLOW)Linking [$@]$(RESET)"
	@$(CC) $(OBJS_CLIENT) $(CFLAGS) $(INCLUDES) -o $@ 
	@printf $(UP)$(CUT)
	@echo "$(GREEN)$(NAME_C) compiled!$(RESET)"

clean:
	@echo "$(YELLOW)cleaning files$(RESET)"
	@$(RM) $(OBJS_DAEMON) $(OBJS_CLIENT)
	@rm -rf $(OBJS_DIR)
	@printf $(UP)$(CUT)
	@echo "$(GREEN)$(NAME) files deleted !$(RESET)"

fclean:	clean
	@echo "$(YELLOW)cleaning files$(RESET)"
	@$(RM) $(NAME_D) $(NAME_C)
	@printf $(UP)$(CUT)
	@echo "$(GREEN)$(NAME) executables deleted !$(RESET)"

re:	fclean all

.PHONY:	all clean fclean re bonus FORCE