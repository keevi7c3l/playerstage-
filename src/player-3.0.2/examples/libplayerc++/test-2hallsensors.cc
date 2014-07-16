/*
 Copyright (c) 2005, Brad Kratochvil, Toby Collett, Brian Gerkey, Andrew Howard, ...
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 * Neither the name of the Player Project nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * goto.cc - a simple (and bad) goto program
 *
 * but, it demonstrates one multi-threaded structure
 *
 * @todo: this has been ported to libplayerc++, but not tested AT ALL
 */

#include <libplayerc++/playerc++.h>

#include <iostream>
#include <stdlib.h> // for atof()
#if !defined (WIN32)
#include <unistd.h>
#endif

#include <math.h>
#include <string>

#include <boost/signal.hpp>
#include <boost/bind.hpp>

#define USAGE \
  "USAGE: goto [-x <x>] [-y <y>] [-h <host>] [-p <port>] [-m]\n" \
  "       -x <x>: set the X coordinate of the target to <x>\n"\
  "       -y <y>: set the Y coordinate of the target to <y>\n"\
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -m       : turn on motors (be CAREFUL!)"

PlayerCc::PlayerClient* robot;
PlayerCc::Position2dProxy* pp;
PlayerCc::LaserProxy* lp;
PlayerCc::HallsensorProxy *bp;
PlayerCc::HallsensorProxy *bp2;
PlayerCc::GripperProxy *gp;

bool gMotorEnable(false);
bool gGotoDone(false);
std::string gHostname(PlayerCc::PLAYER_HOSTNAME);
uint32_t gPort(PlayerCc::PLAYER_PORTNUM);
uint32_t gIndex(0);
uint32_t gDebug(0);
uint32_t gFrequency(10); // Hz

player_pose2d_t gTarget = { 0, 0, 0 };

void print_usage(int argc, char** argv) {
	std::cout << USAGE << std::endl;
}

int parse_args(int argc, char** argv) {
	const char* optflags = "h:p:i:d:u:x:y:m";
	int ch;

	while (-1 != (ch = getopt(argc, argv, optflags))) {
		switch (ch) {
		/* case values must match long_options */
		case 'h':
			gHostname = optarg;
			break;
		case 'p':
			gPort = atoi(optarg);
			break;
		case 'i':
			gIndex = atoi(optarg);
			break;
		case 'd':
			gDebug = atoi(optarg);
			break;
		case 'u':
			gFrequency = atoi(optarg);
			break;
		case 'x':
			gTarget.px = atof(optarg);
			break;
		case 'y':
			gTarget.py = atof(optarg);
			break;
		case 'm':
			gMotorEnable = true;
			break;
		case '?':
		case ':':
		default:
			print_usage(argc, argv);
			return (-1);
		}
	}

	return (0);
}

/*
 * very bad goto.  target is arg (as pos_t*)
 *
 * sets global 'gGotoDone' when it's done
 */
void position_goto(player_pose2d_t target) {
	using namespace PlayerCc;

	double dist, angle;

	dist = sqrt(
			(target.px - pp->GetXPos()) * (target.px - pp->GetXPos())
					+ (target.py - pp->GetYPos())
							* (target.py - pp->GetYPos()));

	angle = atan2(target.py - pp->GetYPos(), target.px - pp->GetXPos());

	double newturnrate = 0;
	double newspeed = 0;

	if (fabs(rtod(angle)) > 10.0) {
		newturnrate = limit((angle / M_PI) * 40.0, -40.0, 40.0);
		newturnrate = dtor(newturnrate);
	} else
		newturnrate = 0.0;

	if (dist > 0.05) {
		newspeed = limit(dist * 0.200, -0.2, 0.2);
	} else
		newspeed = 0.0;

	if (fabs(newspeed) < 0.01)
		gGotoDone = true;

	pp->SetSpeed(newspeed, newturnrate);

}

/*
 * sonar avoid.
 *   policy:
 *     if(object really close in front)
 *       backup and turn away;
 *     else if(object close in front)
 *       stop and turn away
 */
void sonar_avoid(void) {/*
 double min_front_dist = 0.500;
 double really_min_front_dist = 0.300;
 bool avoid = false;

 double newturnrate = 10.0;
 double newspeed = 10.0;

 if ((sp->GetScan(2) < really_min_front_dist) ||
 (sp->GetScan(3) < really_min_front_dist) ||
 (sp->GetScan(4) < really_min_front_dist) ||
 (sp->GetScan(5) < really_min_front_dist))
 {
 avoid = true;
 std::cerr << "really avoiding" << std::endl;
 newspeed = -0.100;
 }
 else if((sp->GetScan(2) < min_front_dist) ||
 (sp->GetScan(3) < min_front_dist) ||
 (sp->GetScan(4) < min_front_dist) ||
 (sp->GetScan(5) < min_front_dist))
 {
 avoid = true;
 std::cerr << "avoiding" << std::endl;
 newspeed = 0;
 }

 if(avoid)
 {
 if ((sp->GetScan(0) + sp->GetScan(1)) < (sp->GetScan(6) + sp->GetScan(7)))
 newturnrate = PlayerCc::dtor(30);
 else
 newturnrate = PlayerCc::dtor(-30);
 }

 if(newspeed < 10.0 && newturnrate < 10.0)
 pp->SetSpeed(newspeed, newturnrate);
 */
}
char sign = 0;
template<typename T>
void Print(T t) {
	if (sign) {
		std::cout << *t << std::endl;
		sign = 0;
	}
}

int main(int argc, char **argv) {
	try {
		using namespace PlayerCc;

		parse_args(argc, argv);
		robot = new PlayerClient(gHostname, gPort);

		bp = new HallsensorProxy(robot, 0);

		bp2 = new HallsensorProxy(robot,1);

		//gp=new GripperProxy(robot, gIndex);
		int i;

		std::cout << "0 exit,1 open,2 close,3 stop,4 store,5 retrive,6 nothing,7 print hall"
				<< std::endl;

		int x;
		for (;;) {
			sign = 1;

			robot->Read();

			//std::cout << *gp << std::endl;

			std::cin >> i;
			switch (i) {
			case 0: {
				return 0;
				break;
			}
			case 1: {
				gp->Open();
				break;
			}
			case 2: {
				gp->Close();
				break;
			}
			case 3: {
				gp->Stop();
				break;
			}
			case 4: {
				gp->Store();
				break;
			}
			case 5: {
				gp->Retrieve();
				break;
			}
			case 6:{
				break;
			}
			case 7:{
				std::cout << *bp << std::endl;
				std::cout << *bp2 <<std::endl;
				break;
			}
			}

		}
	} catch (PlayerCc::PlayerError & e) {
		std::cerr << e << std::endl;
		return -1;
	}

	return (0);
}

