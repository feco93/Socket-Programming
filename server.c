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
#include <string.h>
#include <stdlib.h>

typedef struct player
{
  int fd;			//File descriptor for socket communication.
  char *id;			//Id: kék/piros
  int figures[2];
} PLAYER;

  /**
  * Prints the specified message and then terminate.
  */
void
error (char *msg)
{
  perror (msg);
  exit (1);
}

  /**
  * Indicates whether the game is over.
  */
int
game_over (int figures[])
{
  if (figures[0] > 49 && figures[1] > 49)
    {
      return 1;
    }
  return 0;
}

  /**
  * The first(index) figure beat the second figure if they are on the same field.
  */
void
beat (int firstfigures[], int secondfigures[], int index)
{
  int i;
  for (i = 0; i < 2; ++i)
    {
      if (firstfigures[index] == secondfigures[i])
	{
	  secondfigures[i] = 1;
	  return;
	}
    }
  for (i = 0; i < 2; ++i)
    {
      if (i != index)
	{
	  if (firstfigures[index] == firstfigures[i])
	    {
	      firstfigures[i] = 1;
	      return;
	    }
	}
    }
}

  /**
  * Executes the next loop of the game.
  */
void
nextloop (PLAYER * actualplayer, PLAYER * otherplayer)
{

  int n;
  char buffer[256];
  char answermessage[256];
  int thrown_number;
  int index;

  n = read (actualplayer->fd, buffer, 255);
  if (n < 0)
    error ("Sikertelen olvasás a socketről.");

  //Exits if the client exited
  if (strcmp (buffer, "") == 0)
    {
      bzero (answermessage, 256);
      snprintf (answermessage, 256,
		"A %s játékos lecsatlakazott. A játéknak vége!",
		actualplayer->id);
      n = write (actualplayer->fd, answermessage, strlen (answermessage));
      if (n < 0)
	error ("Sikertelen írás a socketre.");
      n = write (otherplayer->fd, answermessage, strlen (answermessage));
      if (n < 0)
	error ("Sikertelen írás a socketre.");
      exit (0);
    }

  //Exits if the player is surrendered
  if (strstr (buffer, "feladom") != NULL)
    {
      bzero (answermessage, 256);
      snprintf (answermessage, 256,
		"A %s játékos feladta! A játéknak vége!",
		actualplayer->id);
      n = write (actualplayer->fd, answermessage, strlen (answermessage));
      if (n < 0)
	error ("Sikertelen írás a socketre.");
      n = write (otherplayer->fd, answermessage, strlen (answermessage));
      if (n < 0)
	error ("Sikertelen írás a socketre.");
      exit (0);
    }

  //Generate a random number to the specified figure
  thrown_number = rand () % 6 + 1;
  index = atoi (buffer);
  actualplayer->figures[index] += thrown_number;

  //Beat situation
  if (actualplayer->figures[index] <= 49)
    {
      beat (actualplayer->figures, otherplayer->figures, index);
    }

  //Acknowledgement to the client 
  snprintf (answermessage, 256,
	    "A dobott számod: %d\nA játszma állása: a te bábuid: %d, %d  az ellenfél bábui: %d, %d\n",
	    thrown_number, actualplayer->figures[0], actualplayer->figures[1],
	    otherplayer->figures[0], otherplayer->figures[1]);
  n = write (actualplayer->fd, answermessage, strlen (answermessage));
  if (n < 0)
    error ("Sikertelen írás a socketre.");

  //Exits if the game is over
  if (game_over (actualplayer->figures))
    {
      bzero (answermessage, 256);
      snprintf (answermessage, 256,
		"A játszmát a %s játékos nyerte! A játéknak vége!",
		actualplayer->id);
      n = write (actualplayer->fd, answermessage, strlen (answermessage));
      if (n < 0)
	error ("Sikertelen írás a socketre.");
      n = write (otherplayer->fd, answermessage, strlen (answermessage));
      if (n < 0)
	error ("Sikertelen írás a socketre.");
      exit (0);
    }
}

  /**
  * Starts a game.
  */
void
game (PLAYER * blueplayer, PLAYER * redplayer)
{

  while (1)
    {

      nextloop (blueplayer, redplayer);
      nextloop (redplayer, blueplayer);

    }
}

  /**
   * Entry point of the game.
   */
int
main (int argc, char *argv[])
{
  int sockfd, portno, clilen;
  PLAYER blueplayer;
  PLAYER redplayer;
  struct sockaddr_in serv_addr, cli_addr;
  int n;
  char *message;

  if (argc < 2)
    {
      fprintf (stderr, "Hiba, nem adtál meg port számot.\n");
      exit (1);
    }

  /*AF_INET -> Internet Domain, SOCK_STREAM -> Socket stream, 
     0 indicates that the operating system will choose the most appropriate protocol */
  sockfd = socket (AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error ("Hiba a socket létrehozásában.");

  bzero ((char *) &serv_addr, sizeof (serv_addr));
  portno = atoi (argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  //converts a port number in host byte order to a port number in network byte order
  serv_addr.sin_port = htons (portno);

  if (bind (sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0)
    error ("Hiba történt az ip-file descriptor hozzárendelés során.");
  listen (sockfd, 5);
  clilen = sizeof (cli_addr);
  blueplayer.fd = accept (sockfd, (struct sockaddr *) &cli_addr, &clilen);
  if (blueplayer.fd < 0)
    error ("Nem sikerült fogadni a csatlakozást");
  message = "A másik játékos csatlakozására vár a szerver...";
  n = write (blueplayer.fd, message, strlen (message));
  if (n < 0)
    error ("Sikertelen írás a socketre.");
  redplayer.fd = accept (sockfd, (struct sockaddr *) &cli_addr, &clilen);
  if (redplayer.fd < 0)
    error ("Nem sikerült fogadni a csatlakozást");

  blueplayer.id = "kék";
  redplayer.id = "piros";
  blueplayer.figures[0] = 1;
  blueplayer.figures[1] = 1;
  redplayer.figures[0] = 1;
  redplayer.figures[1] = 1;

  message = "Az ön színe: piros\n";
  n = write (redplayer.fd, message, strlen (message));
  if (n < 0)
    error ("Sikertelen írás a socketre.");
  message = "Az ön színe: kék\n";
  n = write (blueplayer.fd, message, strlen (message));
  if (n < 0)
    error ("Sikertelen írás a socketre.");

  message = "A játék elkezdődött! Start!";
  n = write (redplayer.fd, message, strlen (message));
  if (n < 0)
    error ("Sikertelen írás a socketre.");
  n = write (blueplayer.fd, message, strlen (message));
  if (n < 0)
    error ("Sikertelen írás a socketre.");

  game (&blueplayer, &redplayer);

  return 0;
}
