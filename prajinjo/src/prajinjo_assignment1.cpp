/**
 * @prajinjo_assignment1
 * @author  Prajin Jonchhe <prajinjo@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <iostream>
#include <stdio.h>

#include "../include/global.h"
#include "../include/logger.h"

#include <string.h>
#include <stdlib.h>
#include "../include/TCPclient.h"
#include "../include/TCPserver.h"
#include "../include/common.h"
using namespace std;

bool g_isServer;
void setHost(bool isServer);
int isServer();
/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log(argv[2]);

	/* Clear LOGFILE*/
    fclose(fopen(LOGFILE, "w"));

	/*Start Here*/

	/*if(argc <= 5)
	{
		cse4589_print_and_log("Wrong command line arguments");
	}
	else*/
	{
		int listenPort = atoi(argv[2]);
		setListenPort(listenPort);
		if(!strcmp(argv[1], "s"))
		{
			setHost(true);
			runServer();
		}
		else if(!strcmp(argv[1], "c"))
		{
			setHost(false);
			startClient(argv[3]);
			//runClient(argv[3], portNumber);
		}
		else
			cse4589_print_and_log("Wrong host type"); 
	}	
	return 0;
}

void setHost(bool isServer)
{
	g_isServer = isServer;
}

int isServer()
{
	return g_isServer;
}
