   /**********************************************************************
   * Author: Ferenc Sipos
   * 
   * This program is free software: you can redistribute it and/or modify
   * it under the terms of the GNU General Public License as published by
   * the Free Software Foundation, either version 3 of the License, or
   * (at your option) any later version.
   *
   * This program is distributed in the hope that it will be useful,
   * but WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   * GNU General Public License for more details.
   *
   * You should have received a copy of the GNU General Public License
   * along with this program.  If not, see <http://www.gnu.org/licenses/>.
   ***********************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

  /**
   * Prints the specified message and then terminate.
   */
void
error (char *msg)
{
  perror (msg);
  exit (0);
}

int
main (int argc, char *argv[])
{
  int sockfd, portno, n;

  struct sockaddr_in serv_addr;
  struct hostent *server;

  int started = 0;

  char buffer[256];
  if (argc < 3)
    {
      fprintf (stderr, "használat: %s hostname port\n", argv[0]);
      exit (0);
    }
  portno = atoi (argv[2]);
  sockfd = socket (AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error ("Hiba a socket létrehozásában.");
  server = gethostbyname (argv[1]);
  if (server == NULL)
    {
      fprintf (stderr, "Hiba, nem található ilyen nevű host.\n");
      exit (0);
    }
  bzero ((char *) &serv_addr, sizeof (serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy ((char *) server->h_addr,
	 (char *) &serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons (portno);
  if (connect (sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) <
      0)
    error ("Sikertelen csatlakozás.");

  while (1)
    {

      bzero (buffer, 256);
      n = read (sockfd, buffer, 255);
      if (n < 0)
	error ("Sikertelen olvasás a socketről.");
      if (strcmp (buffer, "") == 0)
	{
	  printf ("A szerver lecsatlakazott. A játéknak vége!\n");
	  exit (0);
	}
      printf ("%s\n", buffer);
      if (strstr (buffer, "vége") != NULL)
	{
	  exit (0);
	}

      if (strstr (buffer, "Start") != NULL)
	{
	  started = 1;
	}
      if (started)
	{
	  do
	    {
	      printf
		("Add meg, hogy melyik bábuval szeretnél lépni (0/1)! Vagy válaszd a feladási opciót a (feladom) üzenettel!\n");
	      bzero (buffer, 256);
	      fgets (buffer, 255, stdin);

	    }
	  while (strcmp (buffer, "feladom\n") != 0
		 && strcmp (buffer, "0\n") != 0
		 && strcmp (buffer, "1\n") != 0);
	  n = write (sockfd, buffer, strlen (buffer));
	  if (n < 0)
	    error ("Sikertelen írás a socketre.");
	}

    }
  return 0;
}
