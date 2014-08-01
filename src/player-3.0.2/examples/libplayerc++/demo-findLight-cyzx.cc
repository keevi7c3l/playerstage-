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
PlayerCc::cyzxLaserProxy* lp;
PlayerCc::LightsensorProxy *lspLeft;
PlayerCc::LightsensorProxy *lspRight;

#define RAYS 32
const double minfrontdistance = 1.0;
const double minlfdistance = 0.5;
const double cruisespeed = 0.5;
const double avoidspeed = 0;
const double avoidturn = 0.5;
const double lightturn = 0.4;
const double avoidlrspeed = 0.1;
double reward = false;
const float lightrangemax = 1000;
const double findlightsensorDis = 0.4;

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
		pp = new Position2dProxy(robot,gIndex);
		lp = new cyzxLaserProxy(robot, gIndex);

		PlayerCc::LightsensorProxy *lspLeft;
		PlayerCc::LightsensorProxy *lspRight;

		double minR = 2;
		double minL = 2;
		double minF = 2;
		double newspeed = 0;
		double newturnrate = 0;

		int llc = 0;
		int rlc = 0;
		float lrange = lightrangemax;
		float rrange = lightrangemax;

		// go into read-think-act loop
		for (;;) {
			// this blocks until new data comes; 10Hz by default
			robot->Read();

			if (lp->GetCount() == 22) {
				minL = lp->GetRange(2);
				minR = lp->GetRange(0);
				minF = lp->GetRange(1);
			}
			if ((llc = lspLeft->GetCount()) > 0) {
				for (int i = 0; i < llc; i++) {
					if (lspLeft->GetLight(i).range < lrange)
						lrange = lspLeft->GetLight(i).range;
				}
			} else
				lrange = lightrangemax;
			if ((rlc = lspRight->GetCount()) > 0) {
				for (int i = 0; i < rlc; i++) {
					if (lspRight->GetLight(i).range < rrange)
						rrange = lspRight->GetLight(i).range;
				}
			} else
				rrange = lightrangemax;
			//find the lightsensor , so stop wander
			if(lrange <findlightsensorDis || rrange < findlightsensorDis){
				pp->SetSpeed(0,0);
				sleep(1);
				break;
			}

			std::cout << "minR: " << minR << "minL: " << minL << "minF: "
					<< minF << " left range:" << lrange << " right range:"
					<< rrange << std::endl;

			if (minF > minfrontdistance) {
				if (minL > minfrontdistance && minR > minfrontdistance) {
					//turn to lights
					if (llc > 0 && rlc > 0) {
						newspeed = cruisespeed;
						newturnrate = lrange < rrange ? lightturn : -lightturn;
					} else if (llc > 0) {
						newspeed = cruisespeed;
						newturnrate = lightturn;
					} else if (rlc > 0) {
						newspeed = cruisespeed;
						newturnrate = -lightturn;
					} else {
						newspeed = cruisespeed;
						newturnrate = 0;
					}
				} else if (minL > minR) {
					//turn left
					newspeed = cruisespeed;
					newturnrate = avoidturn;
				} else {
					//turn right
					newspeed = cruisespeed;
					newturnrate = -avoidturn;
				}
			} else if (minL > minfrontdistance && minR > minfrontdistance) {
				newspeed = cruisespeed;
				newturnrate = lrange < rrange ? lightturn : -lightturn;
			} else if (minL > minfrontdistance) {
				newspeed = 0;
				newturnrate = avoidturn;
			} else if (minR > minfrontdistance) {
				newspeed = 0;
				newturnrate = -avoidturn;
			} else {
				pp->SetSpeed(0, 3.14);
				sleep(1);
				newspeed = 0;
				newturnrate = 0;
			}

			std::cout << "speed: " << newspeed << "turn: " << newturnrate
					<< std::endl;

			// write commands to robot
			pp->SetSpeed(newspeed, newturnrate);
		}
	} catch (PlayerCc::PlayerError & e) {
		std::cerr << e << std::endl;
		return -1;
	}

	std::cin>>llc;

	return (0);
}

